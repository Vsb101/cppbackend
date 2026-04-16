#include "../other/file_system.h"

namespace userFileSystem{

using namespace std::literals;


bool IsSubPath(std::filesystem::path path, std::filesystem::path base) {        //Тру если path это подпуть base
    // Приводим оба пути к каноничному виду (без . и ..)
    path = std::filesystem::weakly_canonical(path);
    base = std::filesystem::weakly_canonical(base);

    
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {  //Проверяем, что все компоненты base содержатся внутри path
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

}//namespace filesystem