#version 450
///{
///}

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
  gl_Position = vec4(inPos, 0, 1);
  outColor = inColor;
}