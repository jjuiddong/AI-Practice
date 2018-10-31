
#include "stdafx.h"
#include "dbgai.h"

using namespace ai;

namespace ai
{
	template<class T>
	void ShowActionTree(cAction<T> *node);
}


void ai::ShowDebugWindow()
{
	ImGui::SetNextWindowBgAlpha(0.3f);
	if (ImGui::Begin("AI-Information"))
	{
		for (auto &zealot : g_ai.m_zealots)
		{
			if (ImGui::TreeNode(zealot->m_name.c_str()))
			{
				auto node = zealot->m_brain->m_rootAction;
				ShowActionTree<cZealot>(node);
				ImGui::TreePop();
			}
		}
	}

	ImGui::End();
}


template<class T>
void ai::ShowActionTree(cAction<T> *node)
{
	ImGui::SetNextTreeNodeOpen(true);
	if (ImGui::TreeNode(node->m_name.c_str()))
	{
		for (auto child : node->m_children)
			ShowActionTree<T>(child);
		
		ImGui::TreePop();
	}
}
