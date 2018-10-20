//
// 2018-10-20, jjuiddong
// Unit Action
//		- base action
//
#pragma once


namespace ai
{

	template<class T>
	class cUnitAction : public cAction<T>
		, public common::cMemoryPool<cUnitAction<T>>
	{
	public:
		cUnitAction(T *agent);

		virtual bool StartAction() override;
		virtual bool ActionExecute(const float deltaSeconds) override;
		virtual bool MessageProccess(const sMsg &msg) override;


	public:
		using cAction<T>::m_agent; // base template class member access
	};


	template<class T>
	cUnitAction<T>::cUnitAction(T *agent)
		: cAction<T>(agent, "wait", "", eActionType::WAIT)
	{
	}


	//-----------------------------------------------------------------------
	// StartAction
	//-----------------------------------------------------------------------
	template<class T>
	bool cUnitAction<T>::StartAction()
	{
		return true;
	}


	//-----------------------------------------------------------------------
	// Action Execute
	//-----------------------------------------------------------------------
	template<class T>
	bool cUnitAction<T>::ActionExecute(const float deltaSeconds)
	{
		return true;
	}


	//-----------------------------------------------------------------------
	// MessageProcess
	//-----------------------------------------------------------------------
	template<class T>
	bool cUnitAction<T>::MessageProccess(const sMsg &msg)
	{
		switch (msg.msg)
		{
		case eMsgType::UNIT_SIDEMOVE:
		{
			// 다른 유닛이 이동 중에 충돌이나면, 길을 비켜준다.
			if (cBrain<T> *brain = dynamic_cast<cBrain<T>*>(msg.sender))
			{
				const Vector3 dir = (msg.v - brain->m_agent->m_transform.pos).Normal();

				this->ClearChildAction();
				this->PushAction(new ai::cUnitSideMove<cZealot>(m_agent, brain->m_agent
					, dir, msg.v));
			}
		}
		break;

		default:
			assert(0);
			break;
		}

		return true;
	}

}
