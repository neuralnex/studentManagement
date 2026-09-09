# Enterprise Student Database Management System - Architecture

## Overview

The Student Management System is a layered, enterprise-grade C++ application designed for managing student records, courses, enrollments, results, and associated academic operations in a university setting.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Presentation Layer                       │
│              (CLI, Web UI, REST API - Future)               │
└────────────────────┬────────────────────────────────────────┘
                     │
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│  ┌─────────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ Authentication  │  │  Students    │  │  Courses     │  │
│  │    Service      │  │   Service    │  │   Service    │  │
│  └─────────────────┘  └──────────────┘  └──────────────┘  │
└────────────────────┬────────────────────────────────────────┘
                     │
┌─────────────────────────────────────────────────────────────┐
│                  Business Logic Layer                       │
│  ┌─────────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  Validation     │  │   Rules      │  │   Workflows  │  │
│  │   Business      │  │   Engine     │  │   Processes  │  │
│  └─────────────────┘  └──────────────┘  └──────────────┘  │
└────────────────────┬────────────────────────────────────────┘
                     │
┌─────────────────────────────────────────────────────────────┐
│                  Repository Layer                           │
│  ┌─────────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ IUserRepository │  │IRoleRepository  │IPermission...│  │
│  │      (I)        │  │      (I)     │  │     Repo (I) │  │
│  └────────┬────────┘  └────────┬─────┘  └──────┬───────┘  │
│           │                    │               │           │
│  ┌────────▼────────┐  ┌────────▼─────┐  ┌──────▼───────┐  │
│  │PostgreSQLUser...│  │PostgreSQLRole│  │PostgreSQL...│  │
│  │  Repository     │  │  Repository  │  │ Repository  │  │
│  └─────────────────┘  └──────────────┘  └──────────────┘  │
└────────────────────┬────────────────────────────────────────┘
                     │
┌─────────────────────────────────────────────────────────────┐
│                   Data Access Layer                         │
│  ┌──────────────────────────────────────────────────────┐  │
│  │           Connection Pool (Thread-Safe)              │  │
│  │           Parameterized Query Execution              │  │
│  │           Transaction Management                     │  │
│  └────────────────────┬─────────────────────────────────┘  │
└────────────────────┬────────────────────────────────────────┘
                     │
┌─────────────────────────────────────────────────────────────┐
│                    Database Layer                           │
│              PostgreSQL (Third Normal Form)                 │
│           Tables: Users, Roles, Students, etc.              │
└─────────────────────────────────────────────────────────────┘
```

## Layered Architecture Explanation

### 1. Presentation Layer
- **Responsibility**: User interface and interaction
- **Current**: CLI through main.cpp
- **Future**: REST API, Web UI, Mobile app
- **Isolation**: Does NOT access database or services directly (yet)

### 2. Application Layer
- **Services**: High-level business operations
  - `AuthenticationService`: Login, registration, session management
  - `StudentService`: Student CRUD operations
  - `CourseService`: Course management
  - `EnrollmentService`: Course registration and management
  - `ResultService`: Grade management
  - etc.

### 3. Business Logic Layer
- **Validation**: Input validation, business rule enforcement
- **Rules Engine**: Complex business logic
- **Workflows**: Multi-step processes (enrollment approval, result approval)

### 4. Repository Layer
- **Purpose**: Abstract data access operations
- **Interfaces**: Generic `IRepository<T>` and domain-specific interfaces
- **Implementations**: PostgreSQL-specific implementations
- **Benefit**: Dependency inversion - services depend on abstractions, not concrete implementations

### 5. Data Access Layer
- **Connection Pool**: Thread-safe connection management
- **Query Execution**: Parameterized queries (SQL injection prevention)
- **Transaction Support**: ACID transactions with rollback

### 6. Database Layer
- **Storage**: PostgreSQL relational database
- **Normalization**: 3NF (Third Normal Form)
- **Integrity**: Constraints, triggers, foreign keys

## Key Design Patterns

### 1. Repository Pattern
```cpp
class IRepository<TEntity, TId> {
    virtual TEntity create(const TEntity&) = 0;
    virtual std::optional<TEntity> read(TId) const = 0;
    virtual TEntity update(const TEntity&) = 0;
    virtual bool delete_by_id(TId) = 0;
    // ... pagination, counting, etc.
};
```
**Benefit**: Abstract data access, testable with mocks

### 2. Dependency Injection
```cpp
AuthenticationService::AuthenticationService(
    std::shared_ptr<IUserRepository> userRepo,
    std::shared_ptr<IRoleRepository> roleRepo,
    std::shared_ptr<IPermissionRepository> permRepo
) : m_userRepo(userRepo), m_roleRepo(roleRepo), m_permRepo(permRepo) {}
```
**Benefit**: Loose coupling, easy testing

### 3. RAII (Resource Acquisition Is Initialization)
```cpp
class ConnectionPool::Connection {
public:
    ~Connection() {
        m_pool.releaseConnection(std::move(m_connection));
    }
};
```
**Benefit**: Automatic resource cleanup, exception-safe

### 4. Strategy Pattern
- Different authentication strategies (password, token, OAuth - future)
- Different permission evaluation strategies

### 5. Factory Pattern
- Connection pooling factory
- Repository factory (create appropriate PostgreSQL implementations)

## Core Entities

### Authentication Domain
```
User
├── id
├── email
├── passwordHash
├── firstName
├── lastName
├── roleId
├── status (ACTIVE, INACTIVE, LOCKED, DELETED)
├── failedLoginAttempts
└── lastLoginAt

Role
├── id
├── name
├── description
├── status
└── permissions: List<Permission>

Permission
├── id
├── name
└── description
```

### Academic Domain (Future)
```
Student
├── id
├── userId
├── matricNumber
├── dateOfBirth
├── emergencyContact
├── guardianInfo
└── status

Course
├── id
├── code
├── title
├── creditUnits
├── department
├── lecturer
└── capacity

Enrollment
├── id
├── student
├── course
├── enrollmentDate
└── status

Result
├── id
├── enrollment
├── scores (CA, exam, total)
├── grade
└── status
```

## Role-Based Access Control (RBAC)

### Roles
- **Administrator**: Full system access
- **Registrar**: Student registration, enrollment management
- **Lecturer**: Course teaching, result submission
- **Student**: Self-service access (view results, course registration)
- **Department Officer**: Department-level operations
- **Finance Officer**: Payment processing, financial reports

### Permission Model
- Permissions are granular (e.g., "create_student", "view_results")
- Roles contain multiple permissions
- Users have one role
- Permission evaluation: User -> Role -> Permissions

## Security Considerations

### Password Security
- **Hashing**: bcrypt algorithm with cost factor 12
- **Strength**: Minimum 8 characters, uppercase, lowercase, digit, special character
- **No plaintext**: Never stored or logged

### SQL Injection Prevention
- **Parameterized Queries**: All queries use $1, $2, etc. placeholders
- **Type-safe Binding**: libpqxx parameter binding

### Session Management
- **Token Generation**: Cryptographically random 64-character tokens
- **Expiration**: 1-hour session timeout
- **Invalidation**: Sessions cleared on password change/logout

### Account Lockout
- **Failed Attempts**: Lock after 5 consecutive failed logins
- **Manual Unlock**: Only by admin
- **Logging**: All failed attempts logged to audit trail

### Audit Trail
- **Comprehensive Logging**: Every action recorded
- **Fields**: User, action, entity, old/new values, IP, timestamp
- **Non-repudiation**: Proves who did what when

## Database Design Principles

### Normalization
- **3NF**: Eliminates data redundancy
- **ACID Transactions**: Ensures data consistency
- **Constraints**: Business rules enforced at database level

### Indexes
- Strategic indexing on:
  - Foreign keys
  - Frequently searched fields (email)
  - Status fields
  - Timestamps

### Soft Deletes
- Users/students marked as DELETED, not removed
- Preserves audit trail and referential integrity

## Testing Strategy

### Unit Tests
- **Scope**: Individual classes and functions
- **Mocking**: Mock repositories for service testing
- **Coverage**: Password hashing, validation, business logic

### Integration Tests
- **Scope**: Multiple components working together
- **Database**: Requires running PostgreSQL
- **Scenarios**: User registration → login → permission check

### Test Fixtures
- Seed data for consistent testing
- Database transactions with rollback
- Isolated test environment

## Extensibility Points

### Adding New Modules
1. Create domain entities
2. Define repository interfaces
3. Implement PostgreSQL repositories
4. Create service layer
5. Add business logic
6. Create tests
7. Integrate with dependencies

### Adding Authentication Strategies
```cpp
class IAuthenticationStrategy {
    virtual bool authenticate(const Credentials&) = 0;
};

class PasswordStrategy : public IAuthenticationStrategy { /* ... */ };
class TokenStrategy : public IAuthenticationStrategy { /* ... */ };
class OAuthStrategy : public IAuthenticationStrategy { /* ... */ };
```

## Build and Deployment

### Build System
- **CMake 3.20+**: Cross-platform build configuration
- **C++20**: Modern C++ features
- **Compiler Support**: GCC, Clang, MSVC

### Dependencies
- **PostgreSQL Client**: libpqxx
- **JSON**: nlohmann/json
- **Logging**: spdlog
- **Testing**: Google Test (gtest)
- **Threading**: std::thread (C++11)

### Compilation
```bash
mkdir build
cd build
cmake ..
make
```

### Configuration
- **Environment Variables**: DB_HOST, DB_PORT, DB_NAME, DB_USER, DB_PASSWORD
- **No Hardcoded Secrets**: 12-factor app compliance
- **Logging Levels**: Configurable via environment

## Performance Considerations

### Connection Pooling
- Minimum pool size: half of configured maximum
- Dynamic connection creation up to maximum
- Connection reuse reduces overhead

### Query Optimization
- Prepared statements (future enhancement)
- Strategic indexes
- Pagination for large result sets
- Lazy loading for related data

### Caching (Future)
- Session cache (currently in-memory)
- Permission cache
- Role cache
- Redis integration (planned)

## Monitoring and Observability

### Logging
- **Levels**: debug, info, warn, error
- **Output**: Console + file
- **Format**: timestamp, logger, level, message
- **Structured Logging**: JSON format for machine parsing (future)

### Metrics (Future)
- Request latency
- Database query time
- Cache hit rates
- Error rates

### Health Checks
- Database connectivity
- Connection pool status
- Service dependencies

## Future Enhancements

1. **REST API Layer**: Expose services via HTTP
2. **Web UI**: React/Vue frontend
3. **Advanced Caching**: Redis integration
4. **Distributed Tracing**: OpenTelemetry
5. **Microservices**: Split into domain-based services
6. **Event Streaming**: Kafka for async processing
7. **GraphQL API**: Alternative to REST
8. **Machine Learning**: Predictive analytics
9. **Mobile App**: Native Android/iOS
10. **Container**: Docker/Kubernetes deployment
