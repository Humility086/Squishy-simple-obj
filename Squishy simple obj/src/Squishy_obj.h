#pragma once
#include <vector>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;
namespace sqy_obj //squishy obj parser
{
	static int16_t material_count = -1;
	//==========================================
	//Data types to help parse the obj file data
	//==========================================

	struct attribute_position
	{
		float x;
		float y;
		float z;
		attribute_position operator+(const sqy_obj::attribute_position& other)
		{
			attribute_position result{};
			result.x = this->x + other.x;
			result.y = this->y + other.y;
			result.z = this->z + other.z;

			return result;
		}

		attribute_position operator-(const sqy_obj::attribute_position& other)
		{
			attribute_position result{};
			result.x = this->x - other.x;
			result.y = this->y - other.y;
			result.z = this->z - other.z;

			return result;
		}
	};

	struct attribute_texture_coord
	{
		float u;
		float v;
	};

	struct attribute_normal
	{
		float xn;
		float yn;
		float zn;
	};

	struct Vertex
	{
		attribute_position m_position;
		attribute_texture_coord m_texture_coord;
		attribute_normal m_normal;
	};

	struct Face
	{
		uint16_t indicies[3];
		uint16_t material_index;
	};

	struct Material
	{
		float specular_exponent;
		float ambient_color[3];
		float diffuse_color[3];
		float specular_color[3];
		float alpha;
	};

	struct Mesh
	{
		std::vector<Vertex> m_verticies;
		std::vector<Face> m_face_mapping;
		std::vector<Material> m_materials;
	};

	//==========================================
	//A data collection to help move data around
	//==========================================
	struct Data_manager
	{
		std::vector<sqy_obj::attribute_position> m_positions;
		std::vector<sqy_obj::attribute_texture_coord> m_texture_coords;
		std::vector<sqy_obj::attribute_normal> m_normals;
		//std::vector<sqy_obj::Face> m_faces;
	};


	//==================================
	//Hashing function to allow a switch 
	//statement to work on a string
	//==================================
	constexpr uint32_t switch_statement_values(const std::string_view data_type_specifier) noexcept
	{
		uint32_t hash = 3964;

		for (auto it = data_type_specifier.begin(); it != data_type_specifier.end(); ++it)
		{
			hash = ((hash << 5) + hash) + *it;
		}
		return hash;
	}

	//====================
	//Forward declerations
	//====================

	void create_vertex(sqy_obj::Data_manager& data_manager, std::istringstream& input_stream, sqy_obj::Mesh& mesh);
	bool check_for_duplicates(const int16_t& vertex_pos, const sqy_obj::Mesh& mesh, const sqy_obj::Data_manager& data_manager);
	void triangulate_quads(sqy_obj::Mesh& mesh, const uint32_t(&face_elements)[4], const sqy_obj::Data_manager& data);
	float pos_length_sqaured(const sqy_obj::attribute_position& input);
	void load_material(fs::path source2, sqy_obj::Mesh& mesh);

	//===============================
	//comparison operator overloading
	//===============================
	bool operator==(const sqy_obj::attribute_position& lhs, const sqy_obj::attribute_position& rhs);

	//=============
	//Main function
	//=============

	inline sqy_obj::Mesh load_obj_file(fs::path source)
	{
		sqy_obj::Mesh return_mesh;

		//memory setup
		std::ifstream input(source.string(), std::ios_base::in);
		sqy_obj::Data_manager data_manager;
		std::string current_line_read;

		//parsing logic
		while (std::getline(input, current_line_read))
		{
			std::istringstream input_stream(current_line_read);
			std::string data_type_identifier;
			input_stream >> data_type_identifier;

			switch (switch_statement_values(data_type_identifier))
			{
			case switch_statement_values("v"): case switch_statement_values("V"):
			{
				sqy_obj::attribute_position temp_pos{};
				input_stream >> temp_pos.x >> temp_pos.y >> temp_pos.z;
				data_manager.m_positions.push_back(temp_pos);
				break;
			}
			case switch_statement_values("vt"): case switch_statement_values("VT"):
			{
				sqy_obj::attribute_texture_coord temp_text{};
				input_stream >> temp_text.u >> temp_text.v;
				data_manager.m_texture_coords.push_back(temp_text);
				break;
			}
			case switch_statement_values("vn"): case switch_statement_values("VN"):
			{
				sqy_obj::attribute_normal temp_norm{};
				input_stream >> temp_norm.xn >> temp_norm.yn >> temp_norm.zn;
				data_manager.m_normals.push_back(temp_norm);
				break;
			}
			case switch_statement_values("f"): case switch_statement_values("F"):
			{
				create_vertex(data_manager, input_stream, return_mesh);
				break;
			}
			case switch_statement_values("mtllib"):
			{
				fs::path source2 = source;
				source2.replace_extension(".mtl");
				load_material(source2, return_mesh);
				break;
			}
			case switch_statement_values("usemtl"):
			{
				++material_count;
				break;
			}
			default:
				break;
			}
		}
		return return_mesh;
	}
	//============
	//End function
	//============

	inline void create_vertex(sqy_obj::Data_manager& data_manager, std::istringstream& input_stream, sqy_obj::Mesh& mesh)
	{
		sqy_obj::attribute_texture_coord default_tex{ 0.f, 0.f };
		sqy_obj::attribute_normal default_norm{ 0.f, 0.f, 0.f };

		uint32_t face_elements[4]{ NULL };
		std::string current_face;
		uint16_t loop_count = 0;

		while (input_stream >> current_face)
		{
			std::istringstream active_element(current_face);
			std::string pos{}, tex{}, norm{};
			int16_t p{}, t{}, n{};

			std::getline(active_element, pos, '/');
			std::getline(active_element, tex, '/');
			std::getline(active_element, norm, '/');

			p = std::stoi(pos) - 1;
			t = (!tex.empty()) ? std::stoi(tex) - 1 : -1;
			n = (!norm.empty()) ? std::stoi(norm) - 1 : -1;

			if (check_for_duplicates(p, mesh, data_manager) == false) // vertex does not already exist
			{
				sqy_obj::Vertex temp{};
				//.obj index from 1 not 0
				temp.m_position = data_manager.m_positions[p];
				temp.m_texture_coord = (t != -1) ? data_manager.m_texture_coords[t] : default_tex;
				temp.m_normal = (n != -1) ? data_manager.m_normals[n] : default_norm;
				mesh.m_verticies.push_back(temp);
				face_elements[loop_count] = mesh.m_verticies.size() - 1; //provides the index of the last element pushed in
				++loop_count;
			}
			else //find the index of where the vertex was pushed to
			{
				int32_t index{};
				for (auto it = mesh.m_verticies.begin(); it != mesh.m_verticies.end(); ++it)
				{
					if (it->m_position == data_manager.m_positions[p])
					{
						index = it - mesh.m_verticies.begin();
						face_elements[loop_count] = index;
						++loop_count;
						break;
					}
				}
			}

		}

		if (loop_count == 3)
		{
			sqy_obj::Face temp{};
			temp.indicies[0] = face_elements[0];
			temp.indicies[1] = face_elements[1];
			temp.indicies[2] = face_elements[2];
			temp.material_index = material_count;
			mesh.m_face_mapping.push_back(temp);
			return;
		}

		else
			triangulate_quads(mesh, face_elements, data_manager);
	}

	inline bool check_for_duplicates(const int16_t& vertex_pos, const sqy_obj::Mesh& mesh, const sqy_obj::Data_manager& data_manager)
	{
		for (auto it = mesh.m_verticies.begin(); it != mesh.m_verticies.end(); ++it)
		{
			if (data_manager.m_positions[vertex_pos] == it->m_position)
			{
				return true;
			}
		}
		return false;
	}

	inline void triangulate_quads(sqy_obj::Mesh& mesh, const uint32_t(&face_elements)[4], const sqy_obj::Data_manager& data)
	{
		//Find the longest hypotenuse:
		//There are 4 external lines, using array elements as labels
		//0-1, 1-2, 2-3, 3-4
		//Find the longest abs(length^2) of:
		//a. 0-1 + 1-2
		//b. 1-2 + 2-3
		//c. 2-3 + 3-0
		//d. 3-0 + 0-1

		sqy_obj::attribute_position v0 = mesh.m_verticies[face_elements[0]].m_position;
		sqy_obj::attribute_position v1 = mesh.m_verticies[face_elements[1]].m_position;
		sqy_obj::attribute_position v2 = mesh.m_verticies[face_elements[2]].m_position;
		sqy_obj::attribute_position v3 = mesh.m_verticies[face_elements[3]].m_position;

		sqy_obj::attribute_position l01 = v1 - v0;
		sqy_obj::attribute_position l12 = v2 - v1;
		sqy_obj::attribute_position l23 = v3 - v2;
		sqy_obj::attribute_position l30 = v0 - v3;

		auto l01_sqrd = pos_length_sqaured(l01);
		auto l12_sqrd = pos_length_sqaured(l12);
		auto l23_sqrd = pos_length_sqaured(l23);
		auto l30_sqrd = pos_length_sqaured(l30);

		auto a = l01_sqrd + l12_sqrd;
		auto b = l12_sqrd + l23_sqrd;
		auto c = l23_sqrd + l30_sqrd;
		auto d = l30_sqrd + l01_sqrd;

		float longest = std::max({ a, b, c, d });

		if (longest == a)
		{
			sqy_obj::Face temp{};
			temp.indicies[0] = face_elements[0];
			temp.indicies[1] = face_elements[1];
			temp.indicies[2] = face_elements[2];
			temp.material_index = material_count;
			mesh.m_face_mapping.push_back(temp);
			temp.indicies[1] = face_elements[3];
			mesh.m_face_mapping.push_back(temp);
		}

		else if (longest == b)
		{
			sqy_obj::Face temp{};
			temp.indicies[0] = face_elements[1];
			temp.indicies[1] = face_elements[2];
			temp.indicies[2] = face_elements[3];
			temp.material_index = material_count;
			mesh.m_face_mapping.push_back(temp);
			temp.indicies[1] = face_elements[0];
			mesh.m_face_mapping.push_back(temp);
		}

		else if (longest == c)
		{
			sqy_obj::Face temp{};
			temp.indicies[0] = face_elements[2];
			temp.indicies[1] = face_elements[3];
			temp.indicies[2] = face_elements[0];
			temp.material_index = material_count;
			mesh.m_face_mapping.push_back(temp);
			temp.indicies[1] = face_elements[1];
			mesh.m_face_mapping.push_back(temp);
		}

		else if (longest == d)
		{
			sqy_obj::Face temp{};
			temp.indicies[0] = face_elements[3];
			temp.indicies[1] = face_elements[0];
			temp.indicies[2] = face_elements[1];
			temp.material_index = material_count;
			mesh.m_face_mapping.push_back(temp);
			temp.indicies[1] = face_elements[2];
			mesh.m_face_mapping.push_back(temp);
		}
	}

	//============
	//math helpers
	//============

	inline	float pos_length_sqaured(const sqy_obj::attribute_position& input)
	{
		float m_result = std::abs((input.x * input.x) + (input.y * input.y) + (input.z * input.z));
		return m_result;
	}

	inline bool operator==(const sqy_obj::attribute_position& lhs, const sqy_obj::attribute_position& rhs)
	{
		if (lhs.x != rhs.x)
			return false;

		if (lhs.y != rhs.y)
			return false;

		if (lhs.z != rhs.z)
			return false;

		return true;
	}


	//===============
	//Material parser
	//===============
	inline void load_material(fs::path source2, sqy_obj::Mesh& mesh)
	{
		constexpr float default_output[3]{ 0.f, 0.f, 0.f };
		sqy_obj::Material result{};
		std::ifstream input(source2.string(), std::ios_base::in);
		std::string current_line_read;
		while (std::getline(input, current_line_read))
		{
			std::istringstream input_stream(current_line_read);
			std::string data_type_identifier;
			input_stream >> data_type_identifier;

			switch (switch_statement_values(data_type_identifier))
			{
			case switch_statement_values("Ns"):
			{
				input_stream >> result.specular_exponent;
				break;
			}
			case switch_statement_values("Ka"):
			{
				input_stream >> result.ambient_color[0] >> result.ambient_color[1] >> result.ambient_color[2];
				break;
			}
			case switch_statement_values("Kd"):
			{
				input_stream >> result.diffuse_color[0] >> result.diffuse_color[1] >> result.diffuse_color[2];
				break;
			}
			case switch_statement_values("Ks"):
			{
				input_stream >> result.specular_color[0] >> result.specular_color[1] >> result.specular_color[2];
				break;
			}
			case switch_statement_values("d"):
			{
				input_stream >> result.alpha;
				mesh.m_materials.push_back(result);
			}
			default:
				break;
			}
		}
	}
}
