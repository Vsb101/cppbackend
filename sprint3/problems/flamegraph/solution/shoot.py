import argparse
import subprocess
import time
import random
import shlex
import os
import signal
import sys

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 200
COOLDOWN = 0.15

server_process = None
perf_process = None


def signal_handler(sig, frame):
    print(f'\nSignal {sig} received, cleaning up...')
    cleanup()
    sys.exit(0)


def cleanup():
    global server_process, perf_process
    if perf_process and perf_process.poll() is None:
        print('Stopping perf record...')
        perf_process.send_signal(signal.SIGINT)
        try:
            perf_process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            perf_process.kill()
    
    if server_process and server_process.poll() is None:
        print('Stopping server...')
        server_process.terminate()
        try:
            server_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            server_process.kill()


def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server


def run(command, output=None):
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.DEVNULL)
    return process


def stop(process, wait=False):
    if process and process.poll() is None:
        process.terminate()
        if wait:
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()


def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')


def build_flamegraph(perf_data_file, output_file):
    """Запускает двойной пайп для построения флеймграфа."""
    # Проверяем, что perf.data существует и не пустой
    if not os.path.exists(perf_data_file):
        print(f'ERROR: {perf_data_file} does not exist!')
        return False
    
    file_size = os.path.getsize(perf_data_file)
    print(f'perf.data size: {file_size} bytes')
    
    if file_size == 0:
        print('ERROR: perf.data is empty!')
        return False
    
    # Первый этап: perf script | stackcollapse-perf.pl
    print('Running perf script...')
    perf_script = subprocess.Popen(
        ['perf', 'script', '-f', '-i', perf_data_file],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    
    # stackcollapse-perf.pl читает из stdin
    script_dir = os.path.dirname(os.path.abspath(__file__))
    collapse_script = os.path.join(script_dir, 'FlameGraph', 'stackcollapse-perf.pl')
    print('Running stackcollapse-perf.pl...')
    stack_collapse = subprocess.Popen(
        ['perl', collapse_script],
        stdin=perf_script.stdout,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    perf_script.stdout.close()
    
    # Ждём завершения perf script
    _, perf_stderr = perf_script.communicate(timeout=60)
    if perf_script.returncode != 0:
        print(f'perf script stderr: {perf_stderr.decode()[:500] if perf_stderr else "none"}')
    
    # Второй этап: stackcollapse | flamegraph.pl
    flamegraph_script = os.path.join(script_dir, 'FlameGraph', 'flamegraph.pl')
    print('Running flamegraph.pl...')
    flamegraph = subprocess.Popen(
        ['perl', flamegraph_script],
        stdin=stack_collapse.stdout,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    stack_collapse.stdout.close()
    
    # Получаем результат и записываем в файл
    output, flamegraph_stderr = flamegraph.communicate(timeout=60)
    if flamegraph.returncode != 0:
        print(f'WARNING: flamegraph.pl failed: {flamegraph_stderr.decode()}')
    
    with open(output_file, 'wb') as f:
        f.write(output)
    
    output_size = os.path.getsize(output_file)
    print(f'Flamegraph saved to {output_file} ({output_size} bytes)')
    return True


# Регистрируем обработчик сигналов
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)


server_cmd = start_server()
server_parts = shlex.split(server_cmd)

# Запускаем сервер в фоне
print(f'Starting server: {server_cmd}...')
server_process = subprocess.Popen(server_parts, stderr=subprocess.PIPE)

# Ждём пока сервер запустится
print('Waiting for server to start...')
for i in range(30):
    time.sleep(1)
    result = subprocess.run(['curl', '-s', '-o', '/dev/null', '-w', '%{http_code}', 'localhost:8080/api/v1/maps'], 
                          capture_output=True, text=True)
    if result.returncode == 0 and result.stdout in ['200', '204']:
        print(f'Server is ready (HTTP {result.stdout})')
        break
    if server_process.poll() is not None:
        _, stderr = server_process.communicate()
        print(f'ERROR: Server exited: {stderr.decode() if stderr else "unknown error"}')
        sys.exit(1)
    print(f'Waiting... ({i+1}s)')
else:
    print('WARNING: Server may not be ready, continuing...')

print(f'Server started with PID {server_process.pid}')

perf_file = 'perf.data'

# Запускаем perf record для профилирования существующего процесса
print(f'Starting perf record for PID {server_process.pid}...')
perf_process = subprocess.Popen(
    ['perf', 'record', '-F', '99', '-p', str(server_process.pid), '-o', perf_file, '-g', '--call-graph', 'dwarf,8192'],
    stderr=subprocess.PIPE
)

# Ждём пока perf record начнёт сбор данных
time.sleep(1)

print('Running shots...')
make_shots()

# Даем время на обработку
time.sleep(2)

print('Stopping perf record...')
# Отправляем SIGINT perf record для корректного завершения
perf_process.send_signal(signal.SIGINT)
try:
    perf_process.wait(timeout=30)
except subprocess.TimeoutExpired:
    print('Timeout, killing perf...')
    perf_process.kill()
    perf_process.wait()

# Даем время на запись данных на диск
time.sleep(3)

print('Stopping server...')
server_process.terminate()
try:
    server_process.wait(timeout=5)
except subprocess.TimeoutExpired:
    server_process.kill()

# Проверяем perf.data
if os.path.exists(perf_file):
    size = os.path.getsize(perf_file)
    print(f'perf.data size: {size} bytes')

# Удаляем временные файлы
for f in ['perf.data.map', 'perf.data.old']:
    try:
        os.remove(f)
    except FileNotFoundError:
        pass

output_file = 'graph.svg'
build_flamegraph(perf_file, output_file)

print('Job done')
