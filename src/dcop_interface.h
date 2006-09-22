/*****************************************************************************
 *                                                                           *
 *   Copyright (C) 2005 by Chazal Francois             <neptune3k@free.fr>   *
 *   website : http://workspace.free.fr                                      *
 *                                                                           *
 *                     =========  GPL License  =========                     *
 *    This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the  GNU General Public License as published by   *
 *   the  Free  Software  Foundation ; either version 2 of the License, or   *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *****************************************************************************/

#ifndef DCOP_INTERFACE_H
# define DCOP_INTERFACE_H

//== INCLUDE REQUIREMENTS ===================================================//

/*
** KDE libraries */
#include <dcopobject.h>


//== DEFINE CLASS & DATATYPES ===============================================//

/*
** Class 'DCOPInterface' defines the dcop actions
***************************************************/

class DCOPInterface : virtual public DCOPObject
{
    K_DCOP

k_dcop:
    virtual int     selectedSession() = 0;

    virtual void    slotAddSession() = 0;
    virtual void    slotRemoveSession() = 0;
    virtual void    slotSelectSession(int id) = 0;

    virtual void    slotPasteClipboard() = 0;

    virtual void    slotToggleState() = 0;

    virtual void    slotRenameSession(int id, const QString & name) = 0;
    virtual void    slotSetSessionTitleText(int id, const QString & name) = 0;
    virtual void    slotRunCommandInSession(int id, const QString & value) = 0;
};

#endif /* DCOP_INTERFACE_H */
