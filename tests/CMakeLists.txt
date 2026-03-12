find_package(GTest REQUIRED)

add_executable(test_order_book
    test_order_book.cpp
    ../src/models/order_book.cpp
)

target_include_directories(test_order_book PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_order_book PRIVATE
    GTest::GTest
    GTest::Main
)

add_test(NAME OrderBookTest COMMAND test_order_book)

add_executable(test_metrics_calculator
    test_metrics_calculator.cpp
    ../src/models/order_book.cpp
    ../src/calculator/metrics_calculator.cpp
)

target_include_directories(test_metrics_calculator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_metrics_calculator PRIVATE
    GTest::GTest
    GTest::Main
)

add_test(NAME MetricsCalculatorTest COMMAND test_metrics_calculator)
