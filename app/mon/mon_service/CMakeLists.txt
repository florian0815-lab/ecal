cmake_minimum_required(VERSION 3.20)

project(mon_service)

find_package(eCAL REQUIRED)
find_package(Protobuf REQUIRED)

set(source_files
  mon_service.cpp
)

ecal_add_app_console(${PROJECT_NAME} ${source_files})

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/../../../ecal/core_pb/protobuf
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    eCAL::core
    eCAL::core_pb
    protobuf::libprotobuf
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_14)

ecal_install_app(${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER app/mon)
