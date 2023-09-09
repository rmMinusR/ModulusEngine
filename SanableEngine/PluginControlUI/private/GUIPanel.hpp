#pragma once

#include "GUIContainer.hpp"

#include "Color.h"

namespace GUI
{

	//A Panel is a Container with its own internal canvas. This allows it to enforce bounds and do dirty rendering.

	class Panel : public Container {
	protected:
		Color backgroundColor;

		GraphicsBuffer* internalCanvas;
		GraphicsBufferID internalCanvasID;
		GraphicsBufferManager* internalCanvasOwner;

	public:
		Panel(const ControlID& theID, const Vector2D& relativePosition, const Vector2D& size, const Color& backgroundColor, GraphicsBufferManager* graphicsBufferManager);
		Panel(const ControlID& theID, const Vector2D& relativePosition, const Vector2D& size, const Color& backgroundColor, GraphicsBufferManager* graphicsBufferManager, std::initializer_list<Element*> children);
		virtual ~Panel();

		virtual void draw(GraphicsBuffer* dest) override;
		
		virtual void rebuildAbsoluteTransform(const Vector2D& parentAbsPos) override;
		virtual void rebuildRenderTransform  (const Vector2D& rootRenderPos) override;
		
		inline void setBackgroundColor(const Color& col) { backgroundColor = col; }

		bool isDirty() const override;
		void clearDirty() override;
	};

}