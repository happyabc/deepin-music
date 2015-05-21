#!/usr/bin/python
# -*- coding: utf-8 -*-

import os

from PyQt5.QtCore import (
    Qt, QRect, QUrl,
    pyqtProperty, QObject,
    pyqtSlot, pyqtSignal,
    QThread)
from PyQt5.QtGui import QRegion
from PyQt5.QtQuick import QQuickView
from .basewindow import BaseWindow
from controllers import registerContext, contexts, registerObj
from lrcwindow import LrcWindowManager
from deepin_utils.file import get_parent_dir
from controllers.signalmanager import signalManager


class NewPlaylistWindow(BaseWindow):

    __contextName__ = 'NewPlaylistWindow'

    @registerContext
    def __init__(self, engine=None, parent=None):
        super(NewPlaylistWindow, self).__init__(engine, parent)
        self.moveCenter()

        self.setSource(QUrl.fromLocalFile(
            os.path.join(get_parent_dir(__file__, 2), 'views','dialogs' ,'NewPlaylist.qml')))

        signalManager.newPlaylistWindowShowed.connect(self.show)