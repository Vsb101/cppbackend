# Создать образ на основе базового слоя gcc (там будет ОС и сам компилятор).
# 11.3 — используемая версия gcc.
FROM gcc:11.3 as build
# FROM <image>:<tag> as <name>
# <image> — имя базового образа, например gcc или ubuntu
# <tag> — необязательный параметр, указывает конкретную версию образа - 11.3 
# <name> - имя контейнера

# Выполнить установку зависимостей внутри контейнера.
RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*
# После каждой инструкции RUN создаётся и сохраняется слой контейнера.
# Докер создаёт новый слой после большинства инструкций. 

# Скопировать файлы проекта внутрь контейнера
# COPY <файл/директория> <путь-внутри-контейнера>
# альтернативный вариант записи:
# COPY ["<файл/директория>", "<путь-внутри-контейнера>"]

# копируем conanfile.txt в контейнер и запускаем conan install
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan profile new default --detect && \
    conan profile update settings.compiler.libcxx=libstdc++11 default && \
    conan install .. --build=missing

# только после этого копируем остальные иходники
COPY ./src /app/src
COPY ./tests /app/tests
COPY CMakeLists.txt /app/

# команда для сборки сервера:
RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . -j4

# Контейнер для запуска сервера
FROM ubuntu:22.04 as run

# Создаём пользователя
RUN groupadd -r www && useradd -r -g www www
# USER переключается на указанного пользователя
# Дальнейшие команды выполняются от его имени
USER www

# Копируем в этот контейнер результат работы предыдущего контейнера
COPY --from=build /app/build/bin/game_server /app/
COPY ./data /app/data
COPY ./static /app/static

# Указываем точку входа: собранный и скопированный из контейнера build
# веб-сервер, с указанием параметра config.json
ENTRYPOINT ["/app/game_server", "--config-file", "/app/data/config.json", "--www-root", "/app/static", \
            "--state-file", "state" ]
#            "--state-file", "state", \
#            "--save-state-period", "1000" \
#            ]
