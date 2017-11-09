//
// 2017-10-30, jjuiddong
//
//
#pragma once


class cZealotAI;
class cZealot : public graphic::cModel
			, public ai::iActorInterface<cZealot>
{
public:
	cZealot();
	virtual ~cZealot();

	bool Create(graphic::cRenderer &renderer);
	virtual bool Render(graphic::cRenderer &renderer, const XMMATRIX &parentTm = graphic::XMIdentity, const int flags = 1) override;
	virtual bool Update(graphic::cRenderer &renderer, const float deltaSeconds) override;


	// iActorInterface Override
	virtual Transform& aiGetTransform() override;
	virtual void aiSetAnimation(const Str64 &animationName) override;
	virtual bool aiCollision(const graphic::cBoundingSphere &srcBSphere
		, OUT graphic::cBoundingSphere &collisionSphrere) override;


protected:
	virtual void InitModel(graphic::cRenderer &renderer) override;


public:
	cZealotAI *m_ai;
};


inline Transform& cZealot::aiGetTransform() { return m_transform; }
