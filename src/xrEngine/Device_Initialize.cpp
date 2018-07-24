#include "stdafx.h"
#include "xr_3da/resource.h"
#include <SDL.h>

#include "Include/editor/ide.hpp"
#include "engine_impl.hpp"
#include "GameFont.h"
#include "PerformanceAlert.hpp"
#include "xrCore/ModuleLookup.hpp"

SDL_HitTestResult WindowHitTest(SDL_Window* win, const SDL_Point* area, void* data);

void CRenderDevice::initialize_weather_editor()
{
    m_editor_module = XRay::LoadModule("xrWeatherEditor");
    if (!m_editor_module->IsLoaded())
        return;

    m_editor_initialize = (initialize_function_ptr)m_editor_module->GetProcAddress("initialize");
    VERIFY(m_editor_initialize);

    m_editor_finalize = (finalize_function_ptr)m_editor_module->GetProcAddress("finalize");
    VERIFY(m_editor_finalize);

    m_engine = new engine_impl();
    m_editor_initialize(m_editor, m_engine);
    VERIFY(m_editor);

    //m_hWnd = m_editor->view_handle();
    VERIFY(m_sdlWnd != INVALID_HANDLE_VALUE);

    GEnv.isEditor = true;
}

void CRenderDevice::Initialize()
{
    Log("Initializing Engine...");
    TimerGlobal.Start();
    TimerMM.Start();

    if (strstr(Core.Params, "-weather"))
        initialize_weather_editor();

    R_ASSERT3(SDL_Init(SDL_INIT_VIDEO) == 0, "Unable to initialize SDL", SDL_GetError());

    if (!m_sdlWnd)
    {
        const Uint32 flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN |
            SDL_WINDOW_RESIZABLE
#if SDL_VERSION_ATLEAST(2,0,5)
                | SDL_WINDOW_ALWAYS_ON_TOP
#endif
                | SDL_WINDOW_OPENGL;

        m_sdlWnd = SDL_CreateWindow("S.T.A.L.K.E.R.: Call of Pripyat", 0, 0, 256, 192, flags);
       
        R_ASSERT3(m_sdlWnd, "Unable to create SDL window", SDL_GetError());
        SDL_SetWindowHitTest(m_sdlWnd, WindowHitTest, nullptr);
        SDL_SetWindowMinimumSize(m_sdlWnd, 256, 192);
        xrDebug::SetApplicationWindow(m_sdlWnd);
    }
}

void CRenderDevice::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    font.OutNext("*** ENGINE:   %2.2fms", stats.EngineTotal.result);
    font.OutNext("FPS/RFPS:     %3.1f/%3.1f", stats.fFPS, stats.fRFPS);
    font.OutNext("TPS:          %2.2f M", stats.fTPS);
    if (alert && stats.fFPS < 30)
        alert->Print(font, "FPS       < 30:   %3.1f", stats.fFPS);
}

SDL_HitTestResult WindowHitTest(SDL_Window* /*window*/, const SDL_Point* area, void* /*data*/)
{
    const auto& rect = Device.m_rcWindowClient;

    // size of additional interactive area (in pixels)
    constexpr int hit = 15;

    const bool leftSide = area->x < rect.x + hit;
    const bool topSide = area->y < rect.y + hit;
    const bool bottomSide = area->y > rect.h - hit;
    const bool rightSide = area->x > rect.w - hit;

    if (leftSide && topSide)
        return SDL_HITTEST_RESIZE_TOPLEFT;

    if (rightSide && topSide)
        return SDL_HITTEST_RESIZE_TOPRIGHT;

    if (rightSide && bottomSide)
        return SDL_HITTEST_RESIZE_BOTTOMRIGHT;

    if (leftSide && bottomSide)
        return SDL_HITTEST_RESIZE_BOTTOMLEFT;

    if (topSide)
        return SDL_HITTEST_RESIZE_TOP;

    if (rightSide)
        return SDL_HITTEST_RESIZE_RIGHT;

    if (bottomSide)
        return SDL_HITTEST_RESIZE_BOTTOM;

    if (leftSide)
        return SDL_HITTEST_RESIZE_LEFT;

    const int centerX = rect.w / 2;
    const int centerY = rect.h / 2;

    // Allow drag from any point except window center
    // For this case, 'hit' is a size of a square in the center
    if ((area->x > centerX + hit || area->x < centerX - hit)
        || (area->y > centerY + hit || area->y < centerY - hit))
        return SDL_HITTEST_DRAGGABLE;

    return SDL_HITTEST_NORMAL;
}
