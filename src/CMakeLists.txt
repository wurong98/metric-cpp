add_executable(metric_hyperliquid
    main.cpp
    config/config_loader.cpp
    websocket/client.cpp
    models/order_book.cpp
    calculator/metrics_calculator.cpp
)

target_include_directories(metric_hyperliquid PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
)

target_link_libraries(metric_hyperliquid PRIVATE
    ${yaml-cpp_LIBRARIES}
    ${LIBWEBSOCKETS_LIBRARIES}
    nlohmann_json::nlohmann_json
)
