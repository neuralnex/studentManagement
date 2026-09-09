find_package(GTest REQUIRED)

# Unit tests (use gmock for the service-level tests that mock repositories)
add_executable(sms_unit_tests
    unit/PasswordHasherTest.cpp
    unit/AuthenticationServiceTest.cpp
    unit/StudentServiceTest.cpp
    unit/EnrollmentServiceTest.cpp
)

target_link_libraries(sms_unit_tests PRIVATE
    sms_core
    GTest::GTest
    GTest::Main
    GTest::gmock
    GTest::gmock_main
)

target_compile_features(sms_unit_tests PRIVATE cxx_std_20)

# Add tests to CTest
add_test(
    NAME SMSUnitTests
    COMMAND sms_unit_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Integration tests
add_executable(sms_integration_tests
    integration/DatabaseConnectionTest.cpp
    integration/AuthenticationRepositoryTest.cpp
    integration/StudentRepositoryTest.cpp
    integration/EnrollmentRepositoryTest.cpp
)

target_link_libraries(sms_integration_tests PRIVATE
    sms_core
    GTest::GTest
    GTest::Main
)

target_compile_features(sms_integration_tests PRIVATE cxx_std_20)

add_test(
    NAME SMSIntegrationTests
    COMMAND sms_integration_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
