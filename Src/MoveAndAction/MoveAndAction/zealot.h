//
// 2017-10-30, jjuiddong
//
//
#pragma once


class cZealotBrain;
class cZealot : public graphic::cModel
{
public:
	cZealot();
	virtual ~cZealot();

	bool Create(graphic::cRenderer &renderer);
	virtual bool Render(graphic::cRenderer &renderer, const XMMATRIX &parentTm = graphic::XMIdentity, const int flags = 1) override;
	virtual bool Update(graphic::cRenderer &renderer, const float deltaSeconds) override;


public:
	cZealotBrain *m_ai;
};
