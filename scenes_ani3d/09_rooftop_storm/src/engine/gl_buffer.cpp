#include "gl_buffer.hpp"

using namespace cgp;

void dynamic_vertex_buffer::initialize(std::vector<int> const& attribute_sizes)
{
	clear();
	floats_per_vertex = 0;
	for (int s : attribute_sizes)
		floats_per_vertex += s;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLsizei const stride = GLsizei(floats_per_vertex * sizeof(float));
	size_t offset = 0;
	for (size_t k = 0; k < attribute_sizes.size(); ++k) {
		glEnableVertexAttribArray(GLuint(k));
		glVertexAttribPointer(GLuint(k), attribute_sizes[k], GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offset));
		offset += attribute_sizes[k] * sizeof(float);
	}
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	opengl_check;
}

void dynamic_vertex_buffer::update(std::vector<float> const& data)
{
	vertex_count = floats_per_vertex > 0 ? int(data.size()) / floats_per_vertex : 0;
	size_t const bytes = data.size() * sizeof(float);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	if (bytes > capacity_bytes) {
		capacity_bytes = bytes;
		glBufferData(GL_ARRAY_BUFFER, bytes, data.data(), GL_DYNAMIC_DRAW);
	}
	else if (bytes > 0) {
		glBufferData(GL_ARRAY_BUFFER, capacity_bytes, nullptr, GL_DYNAMIC_DRAW); // orphan
		glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, data.data());
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	opengl_check;
}

void dynamic_vertex_buffer::draw(GLenum primitive) const
{
	if (vertex_count == 0)
		return;
	glBindVertexArray(vao);
	glDrawArrays(primitive, 0, vertex_count);
	glBindVertexArray(0);
	opengl_check;
}

void dynamic_vertex_buffer::clear()
{
	if (vbo != 0) glDeleteBuffers(1, &vbo);
	if (vao != 0) glDeleteVertexArrays(1, &vao);
	vbo = vao = 0;
	vertex_count = 0;
	capacity_bytes = 0;
}
