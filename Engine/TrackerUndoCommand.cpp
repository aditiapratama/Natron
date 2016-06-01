/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "TrackerUndoCommand.h"

#include "Engine/TrackerContext.h"
#include "Engine/Node.h"
#include "Engine/AppInstance.h"


NATRON_NAMESPACE_ENTER;

AddTrackCommand::AddTrackCommand(const TrackMarkerPtr &marker,
                                 const boost::shared_ptr<TrackerContext>& context)
    : UndoCommand()
      , _markers()
      , _context(context)
{
    _markers.push_back(marker);
    setText( tr("Add Track(s)").toStdString() );
}

void
AddTrackCommand::undo()
{
    boost::shared_ptr<TrackerContext> context = _context.lock();

    if (!context) {
        return;
    }

    context->beginEditSelection(TrackerContext::eTrackSelectionInternal);
    for (std::list<TrackMarkerPtr >::const_iterator it = _markers.begin(); it != _markers.end(); ++it) {
        context->removeMarker(*it);
    }
    context->endEditSelection(TrackerContext::eTrackSelectionInternal);
    context->getNode()->getApp()->triggerAutoSave();
}

void
AddTrackCommand::redo()
{
    boost::shared_ptr<TrackerContext> context = _context.lock();

    if (!context) {
        return;
    }
    context->beginEditSelection(TrackerContext::eTrackSelectionInternal);
    context->clearSelection(TrackerContext::eTrackSelectionInternal);

    for (std::list<TrackMarkerPtr >::const_iterator it = _markers.begin(); it != _markers.end(); ++it) {
        context->addTrackToSelection(*it, TrackerContext::eTrackSelectionInternal);
    }
    context->endEditSelection(TrackerContext::eTrackSelectionInternal);
    context->getNode()->getApp()->triggerAutoSave();
}

RemoveTracksCommand::RemoveTracksCommand(const std::list<TrackMarkerPtr > &markers,
                                         const boost::shared_ptr<TrackerContext>& context)
    : UndoCommand()
      , _markers()
      , _context(context)
{
    assert( !markers.empty() );
    for (std::list<TrackMarkerPtr >::const_iterator it = markers.begin(); it != markers.end(); ++it) {
        TrackToRemove t;
        t.track = *it;
        t.prevTrack = context->getPrevMarker(t.track, false);
        _markers.push_back(t);
    }
    setText( tr("Remove Track(s)").toStdString() );
}

void
RemoveTracksCommand::undo()
{
    boost::shared_ptr<TrackerContext> context = _context.lock();

    if (!context) {
        return;
    }
    context->beginEditSelection(TrackerContext::eTrackSelectionInternal);
    context->clearSelection(TrackerContext::eTrackSelectionInternal);
    for (std::list<TrackToRemove>::const_iterator it = _markers.begin(); it != _markers.end(); ++it) {
        int prevIndex = -1;
        TrackMarkerPtr prevMarker = it->prevTrack.lock();
        if (prevMarker) {
            prevIndex = context->getMarkerIndex(prevMarker);
        }
        if (prevIndex != -1) {
            context->insertMarker(it->track, prevIndex);
        } else {
            context->appendMarker(it->track);
        }
        context->addTrackToSelection(it->track, TrackerContext::eTrackSelectionInternal);
    }
    context->endEditSelection(TrackerContext::eTrackSelectionInternal);
    context->getNode()->getApp()->triggerAutoSave();
}

void
RemoveTracksCommand::redo()
{
    boost::shared_ptr<TrackerContext> context = _context.lock();

    if (!context) {
        return;
    }

    TrackMarkerPtr nextMarker = context->getNextMarker(_markers.back().track, true);

    context->beginEditSelection(TrackerContext::eTrackSelectionInternal);
    for (std::list<TrackToRemove>::const_iterator it = _markers.begin(); it != _markers.end(); ++it) {
        context->removeMarker(it->track);
    }
    if (nextMarker) {
        context->addTrackToSelection(nextMarker, TrackerContext::eTrackSelectionInternal);
    }
    context->endEditSelection(TrackerContext::eTrackSelectionInternal);
    context->getNode()->getApp()->triggerAutoSave();
}

NATRON_NAMESPACE_EXIT;
