#include "TypeLayoutView.hpp"

#include "gui/HUD.hpp"
#include "gui/ImageWidget.hpp"

#include "Resources.hpp"

void TypeInfoView::refresh()
{
	//Remove existing
	for (int i = getTransform()->getChildrenCount() - 1; i >= 0; --i)
	{
		hud->removeWidget(getTransform()->getChild(i)->getWidget());
	}
	
	//Repopulate
	target->layout.walkFields(
		[&](const FieldInfo& f)
		{
			size_t cursor = f.offset;
			size_t end = f.offset+f.size;
			size_t nLines = 0;
			while (true)
			{
				size_t lineEnd = std::min(end, (cursor/bytesPerColumn+1)*bytesPerColumn);
				nLines++;

				ImageWidget* w = hud->addWidget<ImageWidget>(nullptr, fieldSprite);
				w->getTransform()->setParent(getTransform());
				AnchoredPositioning* p = w->getTransform()->setPositioningStrategy<AnchoredPositioning>();
				p->snapToCorner( Vector2f(0,0), Vector2f(byteSize.x*(lineEnd-cursor), byteSize.y) );
				p->setCenterByOffsets(byteSize*Vector2f(cursor%bytesPerColumn, cursor/bytesPerColumn), Vector2f(0, 0));
				//p->setRectByOffsets(
				//	Rect<float> {
				//		byteSize*Vector2f(cursor%bytesPerColumn, cursor/bytesPerColumn),
				//		Vector2f(byteSize.x*(lineEnd-cursor), byteSize.y)
				//	},
				//	w->getTransform()
				//);

				cursor = lineEnd;
				if (cursor >= end) break;
			}
		},
		MemberVisibility::All,
		true
	);
}

TypeInfoView::TypeInfoView(HUD* hud, const TypeInfo* target, const UISprite* fieldSprite, const UISprite* parentSprite) :
	Widget(hud),
	target(target),
	fieldSprite(fieldSprite),
	parentSprite(parentSprite)
{
	refresh();
}

TypeInfoView::~TypeInfoView()
{
}

const Material* TypeInfoView::getMaterial() const
{
	return nullptr;
}

void TypeInfoView::renderImmediate(Renderer* target)
{
}
