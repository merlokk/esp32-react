idf_component_register(SRCS "esp32-react.cpp"
                            "espfswrapper.cpp"
                            "mainfactory.cpp"
                            "espconfig.cpp"
                            "espcontrol.cpp"
                       INCLUDE_DIRS ".")

component_compile_options(-std=c++2a)

# Create a SPIFFS image from the contents of the 'spiffs_image' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'.
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)
