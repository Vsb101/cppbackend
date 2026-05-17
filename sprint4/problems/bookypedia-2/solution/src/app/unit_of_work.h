#pragma once

#include <memory>
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "../domain/tag_fwd.h"

namespace app {


enum class TypeOfTransaction{
    Read, Write
};


class UnitOfWork {
public:
    virtual void Commit() = 0;
    virtual domain::AuthorRepository& Authors() = 0;
    virtual domain::BookRepository& Books() = 0;
    virtual domain::TagRepository& Tags() = 0;
protected:
    ~UnitOfWork() = default;
};

class UnitOfWorkFactory {
public:
    virtual std::shared_ptr<UnitOfWork> CreateUnitOfWork(TypeOfTransaction type_of_tr) = 0;
protected:
    ~UnitOfWorkFactory() = default;
};

} // namespace app {