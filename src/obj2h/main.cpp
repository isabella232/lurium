#include <iostream>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h> 

void dump_meshes(const aiScene* scene, const aiNode* node, const char* name, int& counter) {

	for (unsigned n = 0; n < node->mNumMeshes; ++n) {
		const aiMesh* mesh = scene->mMeshes[node->mMeshes[n]];

		std::cout << "// " << mesh->mName.C_Str() << std::endl;
		std::cout << "GLfloat " << name << "_vertices" << counter << "[] = {" << std::endl;

		for (unsigned t = 0; t < mesh->mNumVertices; ++t) {
			aiVector3D tmp = mesh->mVertices[t];
			if (t > 0) {
				std::cout << ", ";
				if ((t % 3) == 0) {
					std::cout << std::endl;
				}
			}
			std::cout << tmp.x << ", " << tmp.y << ", " << tmp.z;
		}

		std::cout << "};" << std::endl << std::endl;

		std::cout << "GLfloat " << name << "_normals" << counter << "[] = {" << std::endl;

		for (unsigned t = 0; t < mesh->mNumVertices; ++t) {
			aiVector3D tmp = mesh->mNormals[t];
			if (t > 0) {
				std::cout << ", ";
				if ((t % 3) == 0) {
					std::cout << std::endl;
				}
			}
			std::cout << tmp.x << ", " << tmp.y << ", " << tmp.z;
		}

		std::cout << "};" << std::endl << std::endl;

		std::cout << "GLushort " << name << "_faces" << counter << "[] = {" << std::endl;

		for (unsigned t = 0; t < mesh->mNumFaces; ++t) {
			aiFace tmp = mesh->mFaces[t];
			if (t > 0) {
				std::cout << ", ";
				if ((t % 3) == 0) {
					std::cout << std::endl;
				}
			}
			for (unsigned i = 0; i < tmp.mNumIndices; ++i) {
				if (i > 0) {
					std::cout << ", ";
				}
				unsigned index = tmp.mIndices[i];
				std::cout << index;
			}
		}

		std::cout << "};" << std::endl;

		counter++;
	}

	for (unsigned n = 0; n < node->mNumChildren; ++n) {
		dump_meshes(scene, node->mChildren[n], name, counter);
	}
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cout << "usage: obj2h 3dfile varname" << std::endl;
		return 1;
	}

	const char* filename = argv[1];
	const char* varname = argv[2];
	// "../glwrap/assets/lowpolytree.3ds"
	auto scene = aiImportFile(filename, aiProcessPreset_TargetRealtime_MaxQuality);
	
	int counter = 0;

	dump_meshes(scene, scene->mRootNode, varname, counter);

}