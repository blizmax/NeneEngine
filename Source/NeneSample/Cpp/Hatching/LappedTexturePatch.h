/*Copyright reserved by KenLee@2020 ken4000kl@gmail.com*/
#ifndef LAPPED_TEXTURE_PATCH
#define LAPPED_TEXTURE_PATCH

#include <set>
#include <deque>
#include <vector>
#include <optional>
#include "NeneEngine/Nene.h"

struct Adjacency
{
	NNUInt face;			// ������
	NNUInt shared_ia;		// ���ߵ�A����
	NNUInt shared_ib;		// ���ߵ�B����
	NNUInt diagonal_ic;		// �ǹ��ߵĶ���
};


class LappedTexturePatch
{
public:
	//
	LappedTexturePatch();
	~LappedTexturePatch();
	//
	void Grow(std::set<NNUInt>& candidate_faces);
	bool IsGrown() { return m_is_grown; }
	void DrawMesh();
	void Initialize(std::shared_ptr<Mesh> source_mesh, std::set<NNUInt>& candidate_faces);
	
private:
	//
	bool IsValidAdjacency(const Adjacency& adjcency);
	//
	void UpdateForRendering();
	//
	std::optional<Adjacency> FindNearestAdjacentFace(std::set<NNUInt>& candidate_faces);

private:
	//
	NNVec3 m_center;
	//
	bool m_is_grown;

	// The patch vertex indices
	std::vector<NNUInt> m_indices;
	std::set<NNUInt> m_indices_set;

	// The source mesh this patch from
	std::shared_ptr<Mesh> m_source_mesh;

	// The patch for rendering on texture
	std::shared_ptr<Mesh> m_patch_mesh;
};


#endif // LAPPED_TEXTURE_PATCH
