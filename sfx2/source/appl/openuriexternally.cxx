/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/system/SystemShellExecute.hpp>
#include <com/sun/star/system/SystemShellExecuteException.hpp>
#include <com/sun/star/system/SystemShellExecuteFlags.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/RuntimeException.hpp>
#include <com/sun/star/uno/XInterface.hpp>
#include <comphelper/processfactory.hxx>
#include <rtl/ustring.h>
#include <rtl/ustring.hxx>
#include <sfx2/app.hxx>
#include <sfx2/sfxresid.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <openuriexternally.hxx>
#include <comphelper/lok.hxx>
#include <LibreOfficeKit/LibreOfficeKitEnums.h>

#include <sfx2/viewsh.hxx>
#include <sfx2/strings.hrc>

bool sfx2::openUriExternally(
    OUString const & uri, bool handleSystemShellExecuteException)
{
    if (comphelper::LibreOfficeKit::isActive())
    {
        if(SfxViewShell* pViewShell = SfxViewShell::Current())
        {
            pViewShell->libreOfficeKitViewCallback(LOK_CALLBACK_HYPERLINK_CLICKED,
                                                   uri.toUtf8().getStr());
            return true;
        }
        return false;
    }

    css::uno::Reference< css::system::XSystemShellExecute > exec(
        css::system::SystemShellExecute::create(comphelper::getProcessComponentContext()));
    try {
        exec->execute(
            uri, OUString(),
            css::system::SystemShellExecuteFlags::URIS_ONLY);
        return true;
    } catch (css::lang::IllegalArgumentException & e) {
        if (e.ArgumentPosition != 0) {
            throw css::uno::RuntimeException(
                "unexpected IllegalArgumentException: " + e.Message);
        }
        SolarMutexGuard g;
        vcl::Window *pWindow = SfxGetpApp()->GetTopWindow();
        std::unique_ptr<weld::MessageDialog> eb(Application::CreateMessageDialog(pWindow ? pWindow->GetFrameWeld() : nullptr,
                                                                 VclMessageType::Warning, VclButtonsType::Ok,
                                                                 SfxResId(STR_NO_ABS_URI_REF)));
        eb->set_primary_text(eb->get_primary_text().replaceFirst("$(ARG1)", uri));
        eb->run();
    } catch (css::system::SystemShellExecuteException & e) {
        if (!handleSystemShellExecuteException) {
            throw;
        }
        SolarMutexGuard g;
        vcl::Window *pWindow = SfxGetpApp()->GetTopWindow();
        std::unique_ptr<weld::MessageDialog> eb(Application::CreateMessageDialog(pWindow ? pWindow->GetFrameWeld() : nullptr,
                                                                 VclMessageType::Warning, VclButtonsType::Ok,
                                                                 SfxResId(STR_NO_WEBBROWSER_FOUND)));
        eb->set_primary_text(
            eb->get_primary_text().replaceFirst("$(ARG1)", uri)
            .replaceFirst("$(ARG2)", OUString::number(e.PosixError))
            .replaceFirst("$(ARG3)", e.Message));
            //TODO: avoid subsequent replaceFirst acting on previous replacement
        eb->run();
    }
    return false;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
