/*Copyright reserved by KenLee@2020 ken4000kl@gmail.com*/
#include <optional>
#include <iostream>
#include "NeneEngine/Debug.h"
#include "LappedTexturePatch.h"

using namespace std;

#define PRECISION 1000000
#define TEXTURE_PASTING_SCALE 1.0f

#define IA(f) (f * 3 + 0)
#define IB(f) (f * 3 + 1)
#define IC(f) (f * 3 + 2)


NNUInt LappedTexturePatch::AddSourceFaceToPatch(const NNUInt& sface)
{
	//
	m_candidate_faces.erase(sface);
	//
	m_coverage_faces.push_back(sface);
	//
	for (size_t si_it = IA(sface); si_it <= IC(sface); ++si_it)
	{
		//
		NNUInt si = m_source_indices[si_it];
		//
		NNUInt pi;
		auto pi_it = m_source_to_patch_index.find(si);
		if (pi_it != m_source_to_patch_index.end())
		{
			pi = pi_it->second;
		}
		else
		{
			pi = NNUInt(m_patch_vertices.size());
			m_source_to_patch_index.insert(make_pair(si, pi));
			Vertex pvertex = m_source_vertices[si];
			pvertex.m_texcoord *= 0.0f;
			m_patch_vertices.emplace_back(pvertex);
		}
		m_patch_indices.push_back(pi);
	}
	//
	return NNUInt((m_patch_indices.size() / 3) - 1);
}

LappedTexturePatch::~LappedTexturePatch()
{}

LappedTexturePatch::LappedTexturePatch(const std::vector<NNUInt>& indices, const std::vector<Vertex>& vertices, const std::vector<std::map<NNUInt, FaceAdjacency>>& face_adjs, std::set<NNUInt>& faces):
	m_source_indices(indices), m_source_vertices(vertices), m_candidate_faces(faces), m_source_face_adjacencies(face_adjs), m_is_grown(false), m_is_optimazed(false)
{
	// 初始面
	// NNUInt face = rand() % NNUInt(candidate_faces.size());
	NNUInt sface = *(m_candidate_faces.begin());
	//
	NNUInt pface = AddSourceFaceToPatch(sface);
	//
	const NNUInt pia = m_patch_indices[IA(pface)];
	const NNUInt pib = m_patch_indices[IB(pface)];
	const NNUInt pic = m_patch_indices[IC(pface)];
	
	// 选择面内一个点
	// m_center_position = RandomPositionInTriangle(m_patch_vertices[pia].m_position, m_patch_vertices[pib].m_position, m_patch_vertices[pic].m_position);
	m_center_position = (m_patch_vertices[pia].m_position + m_patch_vertices[pib].m_position + m_patch_vertices[pic].m_position) / 3.0f;

	// 选择的点对准切线空间的原点
	NNVec3 tagent, bitangent, normal;
	normal = NNNormalize((m_patch_vertices[pia].m_normal + m_patch_vertices[pib].m_normal + m_patch_vertices[pic].m_normal) / 3.0f);
	CalcTangentAndBitangent(m_patch_vertices[pia].m_position, m_patch_vertices[pib].m_position, m_patch_vertices[pic].m_position, normal, tagent, bitangent);

	// 计算纹理坐标 注意最后一个分量应为零
	NNMat3 tbn(tagent, bitangent, normal);
	NNMat3 tbn_inversed = glm::inverse(tbn);
	NNVec3 center_in_tbn = tbn_inversed * m_center_position;
	NNVec3 vertex_a_in_tbn = tbn_inversed * m_patch_vertices[pia].m_position - center_in_tbn;
	NNVec3 vertex_b_in_tbn = tbn_inversed * m_patch_vertices[pib].m_position - center_in_tbn;
	NNVec3 vertex_c_in_tbn = tbn_inversed * m_patch_vertices[pic].m_position - center_in_tbn;
	//
	m_patch_vertices[pia].m_texcoord = (NNVec2(vertex_a_in_tbn.x, vertex_a_in_tbn.y) * TEXTURE_PASTING_SCALE) + NNVec2(0.5f, 0.5f);
	m_patch_vertices[pib].m_texcoord = (NNVec2(vertex_b_in_tbn.x, vertex_b_in_tbn.y) * TEXTURE_PASTING_SCALE) + NNVec2(0.5f, 0.5f);
	m_patch_vertices[pic].m_texcoord = (NNVec2(vertex_c_in_tbn.x, vertex_c_in_tbn.y) * TEXTURE_PASTING_SCALE) + NNVec2(0.5f, 0.5f);
	//
	m_patch_rendering_mesh = Mesh::Create(m_patch_vertices, m_patch_indices, {});
	//
	dLog("[Patch] Init add face: %d; Remain: %zd faces; (%d, %d, %d)", sface, m_candidate_faces.size(), m_source_indices[IA(sface)], m_source_indices[IB(sface)], m_source_indices[IC(sface)]);
}

void LappedTexturePatch::Draw() const
{
	m_patch_rendering_mesh->Draw();
}

void LappedTexturePatch::DrawCoverage() const
{
	//
	if (m_patch_coverage_mesh == nullptr)
	{
		return;
	}
	//
	m_patch_coverage_mesh->Draw();
}

void LappedTexturePatch::GenerateCoverageMesh()
{
	// vertices:
	//		vec3: (SrcTexCoordU, SrcTexCoordV, FaceIndex)
	//		vec2: (PatchTexCoordU, PatchTexCoordV)
	std::vector<NNFloat> vertices(m_coverage_faces.size() * 3 * 5);
	for (size_t fid = 0; fid < m_coverage_faces.size(); ++fid)
	{
		const NNUInt sface = m_coverage_faces[fid];
		for (NNUInt vid = 0; vid < 3; ++vid)
		{
			const NNUInt si = m_source_indices[sface * 3 + vid];
			const Vertex& sv = m_source_vertices[si];
			//
			vertices[(fid * 15) + (vid * 5) + 0] = sv.m_texcoord.x;
			vertices[(fid * 15) + (vid * 5) + 1] = sv.m_texcoord.y;
			//
			vertices[(fid * 15) + (vid * 5) + 2] = float(sface);
			//
			const NNUInt pi = m_source_to_patch_index[si];
			const Vertex pv = m_patch_vertices[pi];
			vertices[(fid * 15) + (vid * 5) + 3] = pv.m_texcoord.x;
			vertices[(fid * 15) + (vid * 5) + 4] = pv.m_texcoord.y;
		}
	}
	m_patch_coverage_mesh = Shape::Create(vertices, NNVertexFormat::POSITION_TEXTURE);
	dLog("[Patch] Regenerate coverage mesh for patch.\n");
}

void LappedTexturePatch::Grow()
{
	if (m_is_grown)
	{
		dLog("[Patch] The patch is grown.\n");
		return;
	}
	optional<NNUInt> pface = AddNearestAdjacentFaceToPatch();
	//
	if (not pface.has_value())
	{
		//
		m_is_grown = true;
		m_patch_rendering_mesh = Mesh::Create(m_patch_vertices, m_patch_indices, {});
		dLog("[Patch] No adjacency found for this patch.\n");
		//
		GenerateCoverageMesh();
	}
	else
	{
		m_patch_rendering_mesh = Mesh::Create(m_patch_vertices, m_patch_indices, {});
	}
}

optional<NNUInt> LappedTexturePatch::AddNearestAdjacentFaceToPatch()
{
	// 找出当前区域的所有相邻
	std::vector<FaceAdjacency> neighbors;
	for (NNUInt src_face : m_coverage_faces)
	{
		for (const auto it : m_source_face_adjacencies[src_face])
		{
			NNUInt dst_face = it.first;
			if (m_candidate_faces.find(dst_face) != m_candidate_faces.end())
			{
				neighbors.push_back(it.second);
			}
		}
	}
	// 找出距离中心最小的合法相邻
	NNFloat min_dis = 3.40281e+038f;
	optional<FaceAdjacency> min_dis_adj = nullopt;
	//
	for (auto& adj : neighbors)
	{
		const NNVec3 pa = m_source_vertices[m_source_indices[IA(adj.dst_face)]].m_position;
		const NNVec3 pb = m_source_vertices[m_source_indices[IB(adj.dst_face)]].m_position;
		const NNVec3 pc = m_source_vertices[m_source_indices[IC(adj.dst_face)]].m_position;
		const NNVec3 center_position = (pa + pb + pc) / 3.0f;
		NNFloat curr_distance = glm::distance(center_position, m_center_position);
		if (curr_distance < min_dis and IsValidAdjacency(adj))
		{
			min_dis = curr_distance;
			min_dis_adj = adj;
		}
	}

	// 计算或复制这个相邻面的纹理坐标
	if (min_dis_adj.has_value())
	{
		// 加到当前区域中
		NNUInt pface = AddSourceFaceToPatch(min_dis_adj->dst_face);
		// 
		const NNUInt src_share_si0 = m_source_indices[SRC_ADJ_SHARE_I0(min_dis_adj->src_face, min_dis_adj->src_edge)];
		const NNUInt src_share_si1 = m_source_indices[SRC_ADJ_SHARE_I1(min_dis_adj->src_face, min_dis_adj->src_edge)];
		const NNUInt src_diago_si2 = m_source_indices[SRC_ADJ_DIAGO_I2(min_dis_adj->src_face, min_dis_adj->src_edge)];
		const NNUInt dst_share_si0 = m_source_indices[DST_ADJ_SHARE_I0(min_dis_adj->dst_face, min_dis_adj->dst_edge)];
		const NNUInt dst_share_si1 = m_source_indices[DST_ADJ_SHARE_I1(min_dis_adj->dst_face, min_dis_adj->dst_edge)];
		const NNUInt dst_diago_si2 = m_source_indices[DST_ADJ_DIAGO_I2(min_dis_adj->dst_face, min_dis_adj->dst_edge)];
		//
		m_patch_vertices[m_patch_indices[m_source_to_patch_index[src_share_si0]]].m_texcoord = m_patch_vertices[m_patch_indices[m_source_to_patch_index[dst_share_si0]]].m_texcoord;
		m_patch_vertices[m_patch_indices[m_source_to_patch_index[src_share_si1]]].m_texcoord = m_patch_vertices[m_patch_indices[m_source_to_patch_index[dst_share_si1]]].m_texcoord;
		m_patch_vertices[m_patch_indices[m_source_to_patch_index[src_diago_si2]]].m_texcoord = m_patch_vertices[m_patch_indices[m_source_to_patch_index[dst_diago_si2]]].m_texcoord;
		// 


		// 检测是否同时与区域的其他三角形相邻

		return pface;
	}

	return nullopt;
}

bool LappedTexturePatch::IsValidAdjacency(const FaceAdjacency& adj)
{
	const NNUInt s_si0 = m_source_indices[DST_ADJ_SHARE_I0(adj.dst_face, adj.dst_edge)];
	const NNUInt s_si1 = m_source_indices[DST_ADJ_SHARE_I0(adj.dst_face, adj.dst_face)];
	const NNUInt d_si2 = m_source_indices[DST_ADJ_DIAGO_I2(adj.dst_face, adj.dst_face)];
}

bool LappedTexturePatch::IsInPatchHull(const NNVec2& ta, const NNVec2& tb)
{
	//
	assert(ta.x != 0.0f or ta.y != 0.0f);
	assert(tb.x != 0.0f or tb.y != 0.0f);
	//
	static const vector<tuple<NNVec2, NNVec2, NNVec2>> polygon_hull = {
		{{0.54248366 * 0.8 + 0.1, (1.0 - 0.02614379) * 0.8 + 0.1}, {0.41993464 * 0.8 + 0.1, (1.0 - 0.02941176) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.41993464 * 0.8 + 0.1, (1.0 - 0.02941176) * 0.8 + 0.1}, {0.35130719 * 0.8 + 0.1, (1.0 - 0.26797386) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.35130719 * 0.8 + 0.1, (1.0 - 0.26797386) * 0.8 + 0.1}, {0.10457516 * 0.8 + 0.1, (1.0 - 0.30228758) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.10457516 * 0.8 + 0.1, (1.0 - 0.30228758) * 0.8 + 0.1}, {0.05882353 * 0.8 + 0.1, (1.0 - 0.33660131) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.05882353 * 0.8 + 0.1, (1.0 - 0.33660131) * 0.8 + 0.1}, {0.06045752 * 0.8 + 0.1, (1.0 - 0.43464052) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.06045752 * 0.8 + 0.1, (1.0 - 0.43464052) * 0.8 + 0.1}, {0.17973856 * 0.8 + 0.1, (1.0 - 0.48366013) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.17973856 * 0.8 + 0.1, (1.0 - 0.48366013) * 0.8 + 0.1}, {0.11111111 * 0.8 + 0.1, (1.0 - 0.73529412) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.11111111 * 0.8 + 0.1, (1.0 - 0.73529412) * 0.8 + 0.1}, {0.14869281 * 0.8 + 0.1, (1.0 - 0.76143791) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.14869281 * 0.8 + 0.1, (1.0 - 0.76143791) * 0.8 + 0.1}, {0.24019608 * 0.8 + 0.1, (1.0 - 0.69607843) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.24019608 * 0.8 + 0.1, (1.0 - 0.69607843) * 0.8 + 0.1}, {0.26797386 * 0.8 + 0.1, (1.0 - 0.75653595) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.26797386 * 0.8 + 0.1, (1.0 - 0.75653595) * 0.8 + 0.1}, {0.20588235 * 0.8 + 0.1, (1.0 - 0.88071895) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.20588235 * 0.8 + 0.1, (1.0 - 0.88071895) * 0.8 + 0.1}, {0.21568627 * 0.8 + 0.1, (1.0 - 0.96405229) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.21568627 * 0.8 + 0.1, (1.0 - 0.96405229) * 0.8 + 0.1}, {0.34640523 * 0.8 + 0.1, (1.0 - 0.97222222) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.34640523 * 0.8 + 0.1, (1.0 - 0.97222222) * 0.8 + 0.1}, {0.52287582 * 0.8 + 0.1, (1.0 - 0.83333333) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.52287582 * 0.8 + 0.1, (1.0 - 0.83333333) * 0.8 + 0.1}, {0.69934641 * 0.8 + 0.1, (1.0 - 0.94281046) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.69934641 * 0.8 + 0.1, (1.0 - 0.94281046) * 0.8 + 0.1}, {0.78431373 * 0.8 + 0.1, (1.0 - 0.92320261) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.78431373 * 0.8 + 0.1, (1.0 - 0.92320261) * 0.8 + 0.1}, {0.84313725 * 0.8 + 0.1, (1.0 - 0.82189542) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.84313725 * 0.8 + 0.1, (1.0 - 0.82189542) * 0.8 + 0.1}, {0.79738562 * 0.8 + 0.1, (1.0 - 0.72058824) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.79738562 * 0.8 + 0.1, (1.0 - 0.72058824) * 0.8 + 0.1}, {0.79738562 * 0.8 + 0.1, (1.0 - 0.63888889) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.79738562 * 0.8 + 0.1, (1.0 - 0.63888889) * 0.8 + 0.1}, {0.93464052 * 0.8 + 0.1, (1.0 - 0.53921569) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.93464052 * 0.8 + 0.1, (1.0 - 0.53921569) * 0.8 + 0.1}, {0.92647059 * 0.8 + 0.1, (1.0 - 0.38562092) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.92647059 * 0.8 + 0.1, (1.0 - 0.38562092) * 0.8 + 0.1}, {0.85130719 * 0.8 + 0.1, (1.0 - 0.31535948) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.85130719 * 0.8 + 0.1, (1.0 - 0.31535948) * 0.8 + 0.1}, {0.86274510 * 0.8 + 0.1, (1.0 - 0.11437908) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.86274510 * 0.8 + 0.1, (1.0 - 0.11437908) * 0.8 + 0.1}, {0.81699346 * 0.8 + 0.1, (1.0 - 0.07843137) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.81699346 * 0.8 + 0.1, (1.0 - 0.07843137) * 0.8 + 0.1}, {0.66339869 * 0.8 + 0.1, (1.0 - 0.10620915) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
		{{0.66339869 * 0.8 + 0.1, (1.0 - 0.10620915) * 0.8 + 0.1}, {0.54248366 * 0.8 + 0.1, (1.0 - 0.02614379) * 0.8 + 0.1}, {0.48202614 * 0.8 + 0.1, (1.0 - 0.37908497) * 0.8 + 0.1}},
	};
	//
	for (const auto& triangle : polygon_hull)
	{
		if (Intersect(ta, tb, get<0>(triangle), get<1>(triangle), get<2>(triangle)) <= IntersectStatus::INTERSECTING)
		{
			return true;
		}
	}
	//
	// dLog("Segment((%f, %f), (%f, %f))\n", ta.x, ta.y, tb.x, tb.y);
	//
	return false;
}
