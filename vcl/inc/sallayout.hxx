/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#ifndef INCLUDED_VCL_INC_SALLAYOUT_HXX
#define INCLUDED_VCL_INC_SALLAYOUT_HXX

#include <iostream>
#include <list>
#include <memory>
#include <vector>

#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <tools/gen.hxx>
#include <vcl/dllapi.h>
#include <vcl/vclenum.hxx> // for typedef sal_UCS4
#include <vcl/devicecoordinate.hxx>

#include "salglyphid.hxx"

#define MAX_FALLBACK 16


class SalGraphics;
class PhysicalFontFace;
struct GlyphItem;
enum class SalLayoutFlags;
namespace vcl {
    class TextLayoutCache;
}

// used for managing runs e.g. for BiDi, glyph and script fallback
class VCL_PLUGIN_PUBLIC ImplLayoutRuns
{
private:
    int                 mnRunIndex;
    std::vector<int>    maRuns;

public:
            ImplLayoutRuns() { mnRunIndex = 0; maRuns.reserve(8); }

    void    Clear()             { maRuns.clear(); }
    void    AddPos( int nCharPos, bool bRTL );
    void    AddRun( int nMinRunPos, int nEndRunPos, bool bRTL );

    bool    IsEmpty() const     { return maRuns.empty(); }
    void    ResetPos()          { mnRunIndex = 0; }
    void    NextRun()           { mnRunIndex += 2; }
    bool    GetRun( int* nMinRunPos, int* nEndRunPos, bool* bRTL ) const;
    bool    GetNextPos( int* nCharPos, bool* bRTL );
    bool    PosIsInRun( int nCharPos ) const;
    bool    PosIsInAnyRun( int nCharPos ) const;
};

class ImplLayoutArgs
{
public:
    // string related inputs
    LanguageTag         maLanguageTag;
    SalLayoutFlags      mnFlags;
    const OUString&     mrStr;
    int                 mnMinCharPos;
    int                 mnEndCharPos;

    // performance hack
    vcl::TextLayoutCache const* m_pTextLayoutCache;

    // positioning related inputs
    const DeviceCoordinate* mpDXArray;     // in pixel units
    DeviceCoordinate    mnLayoutWidth;      // in pixel units
    int                 mnOrientation;      // in 0-3600 system

    // data for bidi and glyph+script fallback
    ImplLayoutRuns      maRuns;
    ImplLayoutRuns      maFallbackRuns;

                ImplLayoutArgs( const OUString& rStr,
                                int nMinCharPos, int nEndCharPos, SalLayoutFlags nFlags,
                                const LanguageTag& rLanguageTag,
                                vcl::TextLayoutCache const* pLayoutCache);

    void        SetLayoutWidth( DeviceCoordinate nWidth )       { mnLayoutWidth = nWidth; }
    void        SetDXArray( const DeviceCoordinate* pDXArray )  { mpDXArray = pDXArray; }
    void        SetOrientation( int nOrientation )  { mnOrientation = nOrientation; }

    void        ResetPos()
                    { maRuns.ResetPos(); }
    bool        GetNextPos( int* nCharPos, bool* bRTL )
                    { return maRuns.GetNextPos( nCharPos, bRTL ); }
    bool        GetNextRun( int* nMinRunPos, int* nEndRunPos, bool* bRTL );
    void        NeedFallback( int nMinRunPos, int nEndRunPos, bool bRTL )
                    { maFallbackRuns.AddRun( nMinRunPos, nEndRunPos, bRTL ); }
    // methods used by BiDi and glyph fallback
    bool        NeedFallback() const
                    { return !maFallbackRuns.IsEmpty(); }
    bool        PrepareFallback();

private:
    void        AddRun( int nMinCharPos, int nEndCharPos, bool bRTL );
};

// For nice SAL_INFO logging of ImplLayoutArgs values
std::ostream &operator <<(std::ostream& s, ImplLayoutArgs const &rArgs);

// all positions/widths are in font units
// one exception: drawposition is in pixel units

// Unfortunately there is little documentation to help implementors of
// new classes derived from SalLayout ("layout engines"), and the code
// and data structures are far from obvious.

// For instance, I *think* the important virtual functions in the
// layout engines are called in this order:

// * InitFont()
// * LayoutText()
// * AdjustLayout(), any number of times (but presumably
// usually not at all or just once)
// * Optionally, DrawText()

// Functions that just return information like GetTexWidth() and
// FillDXArray() are called after LayoutText() and before DrawText().

// Another important questions is which parts of an ImplLayoutArgs can
// be changed by callers between LayoutText() and AdjustLayout()
// calls. It probably makes sense only if one assumes that the "string
// related inputs" part are not changed after LayoutText().

// But why use the same ImplLayoutArgs structure as parameter for both
// LayoutText() and AdjustLayout() in the first place? And why
// duplicate some of the fields in both SalLayout and ImplLayoutArgs
// (mnMinCharPos, mnEndCharPos, mnLayoutFlags==mnFlags,
// mnOrientation)? Lost in history...

class VCL_PLUGIN_PUBLIC SalLayout
{
public:
    virtual         ~SalLayout();
    // used by upper layers
    Point&          DrawBase()                              { return maDrawBase; }
    const Point&    DrawBase() const                        { return maDrawBase; }
    Point&          DrawOffset()                            { return maDrawOffset; }
    const Point&    DrawOffset() const                      { return maDrawOffset; }
    Point           GetDrawPosition( const Point& rRelative = Point(0,0) ) const;

    virtual bool    LayoutText( ImplLayoutArgs& ) = 0;  // first step of layouting
    virtual void    AdjustLayout( ImplLayoutArgs& );    // adjusting after fallback etc.
    virtual void    InitFont() const {}
    virtual void    DrawText( SalGraphics& ) const = 0;

    int             GetUnitsPerPixel() const                { return mnUnitsPerPixel; }
    int             GetOrientation() const                  { return mnOrientation; }

    // methods using string indexing
    virtual sal_Int32 GetTextBreak(DeviceCoordinate nMaxWidth, DeviceCoordinate nCharExtra, int nFactor) const = 0;
    virtual DeviceCoordinate FillDXArray( DeviceCoordinate* pDXArray ) const = 0;
    virtual DeviceCoordinate GetTextWidth() const { return FillDXArray( nullptr ); }
    virtual void    GetCaretPositions( int nArraySize, long* pCaretXArray ) const = 0;
    virtual bool    IsKashidaPosValid ( int /*nCharPos*/ ) const { return true; } // i60594

    // methods using glyph indexing
    virtual int     GetNextGlyphs(int nLen, const GlyphItem** pGlyphs, Point& rPos, int&,
                                  const PhysicalFontFace** pFallbackFonts = nullptr) const = 0;
    virtual bool    GetOutline( SalGraphics&, basegfx::B2DPolyPolygonVector& ) const;
    virtual bool    GetBoundRect( SalGraphics&, tools::Rectangle& ) const;

    // used by glyph+font+script fallback
    virtual void    MoveGlyph( int nStart, long nNewXPos ) = 0;
    virtual void    DropGlyph( int nStart ) = 0;
    virtual void    Simplify( bool bIsBase ) = 0;

    virtual std::shared_ptr<vcl::TextLayoutCache>
        CreateTextLayoutCache(OUString const&) const;

protected:
    // used by layout engines
                    SalLayout();

    static int      CalcAsianKerning( sal_UCS4, bool bLeft, bool bVertical );

private:
                    SalLayout( const SalLayout& ) = delete;
                    SalLayout& operator=( const SalLayout& ) = delete;

protected:
    int             mnMinCharPos;
    int             mnEndCharPos;

    int             mnUnitsPerPixel;
    int             mnOrientation;

    mutable Point   maDrawOffset;
    Point           maDrawBase;
};

class VCL_PLUGIN_PUBLIC MultiSalLayout : public SalLayout
{
public:
    virtual void    DrawText( SalGraphics& ) const override;
    virtual sal_Int32 GetTextBreak(DeviceCoordinate nMaxWidth, DeviceCoordinate nCharExtra, int nFactor) const override;
    virtual DeviceCoordinate FillDXArray( DeviceCoordinate* pDXArray ) const override;
    virtual void    GetCaretPositions( int nArraySize, long* pCaretXArray ) const override;
    virtual int     GetNextGlyphs(int nLen, const GlyphItem** pGlyphs, Point& rPos, int&,
                                  const PhysicalFontFace** pFallbackFonts = nullptr) const override;
    virtual bool    GetOutline( SalGraphics&, basegfx::B2DPolyPolygonVector& ) const override;
    virtual bool    IsKashidaPosValid(int nCharPos) const override;

    // used only by OutputDevice::ImplLayout, TODO: make friend
    explicit        MultiSalLayout( std::unique_ptr<SalLayout> pBaseLayout );
    void            AddFallback( std::unique_ptr<SalLayout> pFallbackLayout,
                                 ImplLayoutRuns const &, const PhysicalFontFace* pFallbackFont );
    virtual bool    LayoutText( ImplLayoutArgs& ) override;
    virtual void    AdjustLayout( ImplLayoutArgs& ) override;
    virtual void    InitFont() const override;

    void SetIncomplete(bool bIncomplete);

public:
    virtual         ~MultiSalLayout() override;

private:
    // dummy implementations
    virtual void    MoveGlyph( int, long ) override {}
    virtual void    DropGlyph( int ) override {}
    virtual void    Simplify( bool ) override {}

                    MultiSalLayout( const MultiSalLayout& ) = delete;
                    MultiSalLayout& operator=( const MultiSalLayout& ) = delete;

private:
    std::unique_ptr<SalLayout> mpLayouts[ MAX_FALLBACK ];
    const PhysicalFontFace* mpFallbackFonts[ MAX_FALLBACK ];
    ImplLayoutRuns  maFallbackRuns[ MAX_FALLBACK ];
    int             mnLevel;
    bool            mbIncomplete;
};

struct GlyphItem
{
    int     mnFlags;
    int     mnCharPos;      // index in string

    int     mnOrigWidth;    // original glyph width
    int     mnNewWidth;     // width after adjustments
    int     mnXOffset;

    sal_GlyphId maGlyphId;
    Point   maLinearPos;    // absolute position of non rotated string

    int     mnFallbackLevel;

public:
            GlyphItem( int nCharPos, sal_GlyphId aGlyphId, const Point& rLinearPos,
                long nFlags, int nOrigWidth )
            :   mnFlags(nFlags)
            ,   mnCharPos(nCharPos)
            ,   mnOrigWidth(nOrigWidth)
            ,   mnNewWidth(nOrigWidth)
            ,   mnXOffset(0)
            ,   maGlyphId(aGlyphId)
            ,   maLinearPos(rLinearPos)
            ,   mnFallbackLevel(0)
            { }

            GlyphItem( int nCharPos, sal_GlyphId aGlyphId, const Point& rLinearPos,
                long nFlags, int nOrigWidth, int nXOffset )
            :   mnFlags(nFlags)
            ,   mnCharPos(nCharPos)
            ,   mnOrigWidth(nOrigWidth)
            ,   mnNewWidth(nOrigWidth)
            ,   mnXOffset(nXOffset)
            ,   maGlyphId(aGlyphId)
            ,   maLinearPos(rLinearPos)
            ,   mnFallbackLevel(0)
            { }

    enum {
        IS_IN_CLUSTER = 0x001,
        IS_RTL_GLYPH  = 0x002,
        IS_DIACRITIC  = 0x004,
        IS_VERTICAL   = 0x008,
        IS_SPACING    = 0x010,
        ALLOW_KASHIDA = 0x020
    };

    bool    IsClusterStart() const  { return ((mnFlags & IS_IN_CLUSTER) == 0); }
    bool    IsRTLGlyph() const      { return ((mnFlags & IS_RTL_GLYPH) != 0); }
    bool    IsDiacritic() const     { return ((mnFlags & IS_DIACRITIC) != 0); }
    bool    IsVertical() const      { return ((mnFlags & IS_VERTICAL) != 0); }
    bool    IsSpacing() const       { return ((mnFlags & IS_SPACING) != 0); }
    bool    AllowKashida() const    { return ((mnFlags & ALLOW_KASHIDA) != 0); }
};

class VCL_PLUGIN_PUBLIC GenericSalLayout : public SalLayout
{
public:
    // used by layout engines
    void            AppendGlyph( const GlyphItem& );
    void            Reserve(int size) { m_GlyphItems.reserve(size + 1); }
    virtual void    ApplyDXArray(ImplLayoutArgs&) = 0;
    void            Justify(DeviceCoordinate nNewWidth);
    void            ApplyAsianKerning(const OUString& rStr);

    // used by upper layers
    virtual DeviceCoordinate GetTextWidth() const override;
    virtual DeviceCoordinate FillDXArray( DeviceCoordinate* pDXArray ) const override;
    virtual sal_Int32 GetTextBreak(DeviceCoordinate nMaxWidth, DeviceCoordinate nCharExtra, int nFactor) const override;
    virtual void    GetCaretPositions( int nArraySize, long* pCaretXArray ) const override;

    // used by display layers
    virtual int     GetNextGlyphs(int nLen, const GlyphItem** pGlyphs, Point& rPos, int&,
                                  const PhysicalFontFace** pFallbackFonts = nullptr) const override;

protected:
                    GenericSalLayout();
    virtual         ~GenericSalLayout() override;

    // for glyph+font+script fallback
    virtual void    MoveGlyph( int nStart, long nNewXPos ) override;
    virtual void    DropGlyph( int nStart ) override;
    virtual void    Simplify( bool bIsBase ) override;

    virtual bool    GetCharWidths(DeviceCoordinate* pCharWidths) const = 0;

    std::vector<GlyphItem>     m_GlyphItems;

private:
                    GenericSalLayout( const GenericSalLayout& ) = delete;
                    GenericSalLayout& operator=( const GenericSalLayout& ) = delete;
};

#undef SalGraphics

#endif // INCLUDED_VCL_INC_SALLAYOUT_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
