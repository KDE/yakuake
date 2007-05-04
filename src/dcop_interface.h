/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2005 Francois Chazal <neptune3k@free.fr>
  Copyright (C) 2006-2007 Eike Hein <hein@kde.org>
*/

#ifndef DCOP_INTERFACE_H
# define DCOP_INTERFACE_H


#include <dcopobject.h>


class DCOPInterface : virtual public DCOPObject
{
    K_DCOP

    k_dcop:
        virtual void slotToggleState() = 0;

        virtual void slotAddSession() = 0;

        virtual void slotRemoveSession() = 0;
        virtual void slotRemoveSession(int session_id) = 0;

        virtual void slotRemoveTerminal() = 0;
        virtual void slotRemoveTerminal(int session_id) = 0;
        virtual void slotRemoveTerminal(int session_id, int terminal_id) = 0;

        virtual void slotRenameSession(int session_id, const QString & name) = 0;

        virtual int tabPositionForSessionId(int session_id) = 0;
        virtual int sessionIdForTabPosition(int position) = 0;

        virtual int selectedSession() = 0;
        virtual int selectedTerminal() = 0;
        virtual void slotSelectSession(int session_id) = 0;
        virtual void slotSelectTabPosition(int position) = 0;

        virtual const QString sessionTitle() = 0;
        virtual const QString sessionTitle(int session_id) = 0;
        virtual const QString sessionTitle(int session_id, int terminal_id) = 0;

        virtual void slotSetSessionTitleText(const QString& title) = 0;
        virtual void slotSetSessionTitleText(int session_id, const QString& title) = 0;
        virtual void slotSetSessionTitleText(int session_id, int terminal_id, const QString& title) = 0;

        virtual void slotPasteClipboard() = 0;
        virtual void slotPasteClipboard(int session_id) = 0;
        virtual void slotPasteClipboard(int session_id, int terminal_id) = 0;

        virtual void slotPasteSelection() = 0;
        virtual void slotPasteSelection(int session_id) = 0;
        virtual void slotPasteSelection(int session_id, int terminal_id) = 0;

        virtual void slotRunCommandInSession(const QString& command) = 0;
        virtual void slotRunCommandInSession(int session_id, const QString& command) = 0;
        virtual void slotRunCommandInSession(int session_id, int terminal_id, const QString& command) = 0;

        virtual void slotSplitHorizontally() = 0;
        virtual void slotSplitHorizontally(int session_id) = 0;
        virtual void slotSplitHorizontally(int session_id, int terminal_id) = 0;

        virtual void slotSplitVertically() = 0;
        virtual void slotSplitVertically(int session_id) = 0;
        virtual void slotSplitVertically(int session_id, int terminal_id) = 0;
};

#endif /* DCOP_INTERFACE_H */
