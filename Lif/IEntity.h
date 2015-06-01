#pragma once

#include "I2DRenderer.h"
#include "IObservable.h"
#include "Rect.h"

struct ID2D1Factory;
struct IDWriteFactory;
struct ID2D1RenderTarget;

namespace tod
{
	class IEntity : public IObservable
	{
	protected:
		// The world rect for this component - x, y position & relative w,h bounds
		SRect m_Rect;
	public:
		IEntity( );
		virtual ~IEntity( );

		virtual bool AddObserver( const std::weak_ptr<IObserver>& observer, uint32 type ) = 0;

		// Create or delete and recreate resources
		//virtual bool CreateResources( ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory ) = 0;
		//virtual bool CreateDeviceResources( ID2D1RenderTarget* pD2DRenderTarget ) = 0;
		virtual bool CreateResources( I2DRenderer* pRenderer ) = 0;
		virtual bool CreateDeviceResources( I2DRenderer* pRenderer ) = 0;
		virtual void DestroyResources( ) = 0;
		virtual void DestroyDeviceResources( ) = 0;

		// Draw the control
		//virtual void Draw( ID2D1RenderTarget* pD2DRenderTarget ) = 0;
		//virtual void PartialDraw( ID2D1RenderTarget* pD2DRenderTarget, const SUIRect& subRect ) = 0;
		virtual void Draw( I2DRenderer* pRenderer ) = 0;
		virtual void PartialDraw( I2DRenderer* pRenderer, const SRect& subRect ) = 0;
	
	protected:
		virtual void Notify( uint32 eventType, IEventArgs* eventArgs ) = 0;
	};
}
