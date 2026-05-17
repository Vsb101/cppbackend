#pragma once
#include <string>

#include "book.h"

namespace domain {

class Tag {
public:
    Tag(domain::BookId book_id, std::string tag)
        : book_id_(std::move(book_id))
        , tag_(std::move(tag)){
    }

    const domain::BookId& GetBookId() const noexcept {
        return book_id_;
    }

    const std::string& GetTag() const noexcept {
        return tag_;
    }

private:
    domain::BookId book_id_;
    std::string tag_;

};

class TagRepository {
public:
    virtual void Save(const std::vector<Tag>& tags) = 0;
    virtual void Save(const std::vector<std::string>& tags, const std::string& book_id) = 0;
    virtual void DeleteTagsForAuthor(const std::string& author_id) = 0;
    virtual void DeleteTagsForBook(const std::string& book_id) = 0;
    virtual std::vector<std::string> GetTags(const std::string& book_id) const = 0;
protected:
    ~TagRepository() = default;
};

}  // namespace domain
