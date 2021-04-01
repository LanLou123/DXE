#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

class ShaderProgram {
public:
	ShaderProgram();
	ShaderProgram(const ShaderProgram& rhs) = delete;
	ShaderProgram& operator=(const ShaderProgram& rhs) = delete;
	~ShaderProgram() = default;
};

#endif // !SHADER_PROGRAM_H
