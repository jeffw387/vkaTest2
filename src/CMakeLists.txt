add_custom_target(shader_compilation ALL)
add_dependencies(main shader_compilation)

function(process_shader shader_name shader_stage)
  set(input_json ${CMAKE_CURRENT_LIST_DIR}/${shader_name}.json)
  set(input_glsl ${CMAKE_CURRENT_LIST_DIR}/${shader_name})
  set(output_json ${CMAKE_BINARY_DIR}/bin/${shader_name}.json)
  set(output_glsl ${CMAKE_BINARY_DIR}/bin/${shader_name})
  set(output_spv ${CMAKE_BINARY_DIR}/bin/${shader_name}.spv)
  add_custom_command(DEPENDS ${input_json} ${input_glsl}
    OUTPUT ${output_glsl}
    COMMAND json-shader -s ${shader_stage} ${input_json} ${input_glsl} ${output_glsl})
  add_custom_command(DEPENDS ${output_glsl}
    OUTPUT ${output_spv}
    COMMAND glslangValidator -V ${output_glsl} -o ${output_spv})
  add_custom_command(DEPENDS ${input_json}
    OUTPUT ${output_json}
    COMMAND cmake -E copy_if_different ${input_json} ${output_json})
  add_custom_target(${shader_name}-gen DEPENDS ${output_spv} ${output_json})
  add_dependencies(shader_compilation ${shader_name}-gen)
endfunction()

process_shader(shader.vert vertex)
process_shader(shader.frag fragment)