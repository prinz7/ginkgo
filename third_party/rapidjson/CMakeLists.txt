ginkgo_load_git_package(rapidjson_external
    "https://github.com/Tencent/rapidjson.git"
    "8d272e53a4d1dc405e08ce2dd50159c58f4451e9"
    "-DRAPIDJSON_BUILD_DOC=OFF" "-DRAPIDJSON_BUILD_EXAMPLES=OFF"
    "-DRAPIDJSON_BUILD_TESTS=OFF" "-DRAPIDJSON_BUILD_THIRDPARTY_GTEST=OFF"
    "-DRAPIDJSON_BUILD_CXX11=ON")
# Remove directory with questionable licensing
file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/src/bin/jsonchecker)
ginkgo_add_external_target(rapidjson rapidjson src/include "" STATIC "" rapidjson_external TRUE)
