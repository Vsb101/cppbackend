#pragma once

#include <pqxx/connection>
#include <mutex>
#include <condition_variable>
#include <queue> // Заменяем vector на queue для правильной FIFO логики
#include <memory>
#include <cassert>

namespace infra {

class ConnectionPool {
    using PoolType = ConnectionPool;
    using ConnectionPtr = std::shared_ptr<pqxx::connection>;

public:
    class ConnectionWrapper {
    public:
        ConnectionWrapper(ConnectionPtr&& conn, PoolType& pool) noexcept
            : conn_{std::move(conn)}
            , pool_{&pool} {
        }

        ConnectionWrapper(const ConnectionWrapper&) = delete;
        ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

        ConnectionWrapper(ConnectionWrapper&&) = default;
        ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

        pqxx::connection& operator*() const& noexcept {
            return *conn_;
        }
        pqxx::connection& operator*() const&& = delete;

        pqxx::connection* operator->() const& noexcept {
            return conn_.get();
        }

        ~ConnectionWrapper() {
            if (conn_) {
                pool_->ReturnConnection(std::move(conn_));
            }
        }

    private:
        ConnectionPtr conn_;
        PoolType* pool_;
    };

    template <typename ConnectionFactory>
    ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory) {
        // Заполняем очередь созданными соединениями
        for (size_t i = 0; i < capacity; ++i) {
            pool_.emplace(connection_factory());
        }
    }

    ConnectionWrapper GetConnection() {
        std::unique_lock lock{mutex_};
        // Ждем, пока в очереди появится хотя бы одно свободное соединение
        cond_var_.wait(lock, [this] {
            return !pool_.empty();
        });
        
        // Извлекаем соединение из начала очереди
        auto conn = std::move(pool_.front());
        pool_.pop();
        
        return {std::move(conn), *this};
    }

private:
    void ReturnConnection(ConnectionPtr&& conn) {
        {
            std::lock_guard lock{mutex_};
            // Возвращаем соединение в конец очереди
            pool_.push(std::move(conn));
        }
        cond_var_.notify_one();
    }

    std::mutex mutex_;
    std::condition_variable cond_var_;
    std::queue<ConnectionPtr> pool_; // Очередь гарантирует безопасность перемещения shared_ptr
};

}  // namespace infra
