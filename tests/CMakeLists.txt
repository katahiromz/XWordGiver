subdirs(black_place)

add_test(NAME black_place_1 COMMAND $<TARGET_FILE:black_place> 11 11 300)
add_test(NAME black_place_2 COMMAND $<TARGET_FILE:black_place> 12 12 30)
