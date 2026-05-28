add_executable(test_suite main.cpp)
target_link_libraries(test_suite xreflect)
set_target_properties(test_suite PROPERTIES FOLDER "examples")
