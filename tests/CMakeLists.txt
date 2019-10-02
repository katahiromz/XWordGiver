subdirs(black_place tiling)

add_test(NAME black_place_1 COMMAND $<TARGET_FILE:black_place> 11 11 300)
add_test(NAME black_place_2 COMMAND $<TARGET_FILE:black_place> 12 12 30)

add_test(NAME tiling_1 COMMAND $<TARGET_FILE:tiling> 11 11 300)
add_test(NAME tiling_2 COMMAND $<TARGET_FILE:tiling> 12 12 30)
