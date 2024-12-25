#include "TypeLayoutView.hpp"

#include <sstream>

#include "ShaderProgram.hpp"
#include "Material.hpp"
#include "gui/HUD.hpp"
#include "gui/ImageWidget.hpp"
#include "gui/LabelWidget.hpp"
#include "gui/VerticalGroupWidget.hpp"

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
			size_t fieldHash = std::hash<std::string>{}(f.name) ^ std::hash<TypeName>{}(f.type) ^ std::hash<TypeName>{}(f.owner);
			float hue = (fieldHash & 0xffffULL)/float(0xffffULL);
			SDL_Color color {
				uint8_t(255*( 1-1.5f*std::min(abs(hue), abs(hue-1)) )),
				uint8_t(255*( 1-1.5f*abs(hue-0.33f) )),
				uint8_t(255*( 1-1.5f*abs(hue-0.66f) )),
				255
			};

			// Background image(s)
			size_t cursor = f.offset;
			size_t end = f.offset+f.size;
			size_t nLines = 0;
			while (true)
			{
				size_t lineEnd = std::min(end, (cursor/bytesPerColumn+1)*bytesPerColumn);
				nLines++;

				ImageWidget* w = hud->addWidget<ImageWidget>(imageMat, fieldSprite);
				w->getTransform()->setParent(getTransform());
				w->setTintColor(color.r, color.g, color.b, color.a);
				AnchoredPositioning* p = w->getTransform()->setPositioningStrategy<AnchoredPositioning>();
				p->snapToCorner( Vector2f(0,0), Vector2f(byteSize.x*(lineEnd-cursor), byteSize.y) );
				p->setCenterByOffsets(byteSize*Vector2f(cursor%bytesPerColumn, cursor/bytesPerColumn), Vector2f(0, 0));
				//p->setLocalRectByOffsets(
				//	Rect<float> {
				//		byteSize*Vector2f(cursor%bytesPerColumn, cursor/bytesPerColumn),
				//		Vector2f(byteSize.x*(lineEnd-cursor), byteSize.y)
				//	},
				//	getTransform()
				//);

				cursor = lineEnd;
				if (cursor >= end) break;
			}

			//Add label container
			VerticalGroupWidget* lblContainer = hud->addWidget<VerticalGroupWidget>();
			lblContainer->getTransform()->setParent( getTransform()->getChild(getTransform()->getChildrenCount()-nLines) );
			lblContainer->getTransform()->setPositioningStrategy<AnchoredPositioning>()->fillParent();
			lblContainer->getTransform()->setRelativeRenderDepth(1);

			//Add name label
			LabelWidget* nameLbl = hud->addWidget<LabelWidget>(textMat, textFont);
			{
				std::stringstream ss;
				ss << f.owner.as_str() << "::" << f.name;
				nameLbl->setText(ss.str());
			}
			nameLbl->align = Vector2f(0.5f, 0.5f);
			nameLbl->getTransform()->setParent(lblContainer->getTransform());
			nameLbl->getTransform()->setPositioningStrategy<AutoLayoutPositioning>(lblContainer);

			//Add type label
			LabelWidget* typeLbl = hud->addWidget<LabelWidget>(textMat, textFont);
			typeLbl->setText(f.type.as_str());
			typeLbl->align = Vector2f(0.5f, 0.5f);
			typeLbl->getTransform()->setParent(lblContainer->getTransform());
			typeLbl->getTransform()->setPositioningStrategy<AutoLayoutPositioning>(lblContainer);
		},
		MemberVisibility::All,
		true
	);

	bool isLittleEndian;
	{
		int n = 1;
		isLittleEndian = (*(char*)&n == 1);
	}

	//Repopulate: implicit values (vptrs)
	target->layout.walkImplicits(
		[&](const TypeInfo::Layout::ImplicitInfo& imp)
		{
			//Add label
			LabelWidget* lbl = hud->addWidget<LabelWidget>(textMat, textFont);
			if (imp.data)
			{
				std::stringstream tmp;
				tmp << "[vptr: 0x";
				
				const char* hexLut = "0123456789abcdef";
				if (isLittleEndian)
				{
					for (int i = imp.size-1; i >= 0; --i)
					{
						//Each byte of binary expands to 2 of hex
						tmp << hexLut[imp.data[i]>>4 & 0xf];
						tmp << hexLut[imp.data[i] & 0xf];
					}
				}
				else
				{
					for (int i = 0; i < imp.size; ++i)
					{
						//Each byte of binary expands to 2 of hex
						tmp << hexLut[imp.data[i]>>4 & 0xf];
						tmp << hexLut[imp.data[i] & 0xf];
					}
				}

				tmp << "]";
				lbl->setText(tmp.str());
			}
			else
			{
				lbl->setText("[padding]");
			}
			lbl->align = Vector2f(0.5f, 0.5f);
			
			//Simple fill to first line
			lbl->getTransform()->setParent( getTransform() );
			lbl->getTransform()->setRelativeRenderDepth(1);
			AnchoredPositioning* pos = lbl->getTransform()->setPositioningStrategy<AnchoredPositioning>();
			pos->snapToCorner( Vector2f(0,0), Vector2f(byteSize.x*imp.size, byteSize.y) );
			pos->setCenterByOffsets(byteSize*Vector2f(imp.offset%bytesPerColumn, imp.offset/bytesPerColumn), Vector2f(0, 0));
		}
	);
}

TypeInfoView::TypeInfoView(HUD* hud, const TypeInfo* target, const Material* imageMat, const UISprite* fieldSprite, const UISprite* parentSprite, const Material* textMat, const Font* textFont) :
	Widget(hud),
	target(target),
	imageMat(imageMat),
	fieldSprite(fieldSprite),
	parentSprite(parentSprite),
	textMat(textMat),
	textFont(textFont)
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
