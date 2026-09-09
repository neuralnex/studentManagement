#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <cstdint>

namespace sms::database::repositories {

/**
 * @brief Generic repository interface
 * 
 * Defines the contract for data access operations. All repositories
 * must implement this interface to ensure consistency and testability.
 * 
 * @tparam TEntity Entity type
 * @tparam TId ID type
 */
template<typename TEntity, typename TId = int64_t>
class IRepository {
public:
    virtual ~IRepository() = default;

    /**
     * @brief Create/persist entity
     * @param entity Entity to create
     * @return Created entity with assigned ID
     * @throws std::runtime_error on database error
     */
    virtual TEntity create(const TEntity& entity) = 0;

    /**
     * @brief Read entity by ID
     * @param id Entity ID
     * @return Entity if found, std::nullopt otherwise
     * @throws std::runtime_error on database error
     */
    virtual std::optional<TEntity> read(TId id) const = 0;

    /**
     * @brief Update existing entity
     * @param entity Entity with updated data
     * @return Updated entity
     * @throws std::runtime_error if entity not found or database error
     */
    virtual TEntity update(const TEntity& entity) = 0;

    /**
     * @brief Delete entity by ID
     * @param id Entity ID
     * @return True if deleted, false if not found
     * @throws std::runtime_error on database error
     */
    virtual bool delete_by_id(TId id) = 0;

    /**
     * @brief Get all entities
     * @return Vector of all entities
     * @throws std::runtime_error on database error
     */
    virtual std::vector<TEntity> readAll() const = 0;

    /**
     * @brief Get entities with pagination
     * @param offset Starting position
     * @param limit Maximum results
     * @return Vector of entities
     * @throws std::runtime_error on database error
     */
    virtual std::vector<TEntity> readPage(int64_t offset, int64_t limit) const = 0;

    /**
     * @brief Count total entities
     * @return Total count
     * @throws std::runtime_error on database error
     */
    virtual int64_t count() const = 0;

    /**
     * @brief Check if entity exists
     * @param id Entity ID
     * @return True if exists
     * @throws std::runtime_error on database error
     */
    virtual bool exists(TId id) const = 0;
};

} // namespace sms::database::repositories
