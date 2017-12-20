//
// Copyright (C) 2012~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#include "editor.h"
#include "batchdialog.h"
#include "editordialog.h"
#include "filelistmodel.h"
#include "model.h"
#include <QCloseEvent>
#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QtConcurrentRun>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/standardpath.h>

namespace fcitx {

ListEditor::ListEditor(QWidget *parent)
    : FcitxQtConfigUIWidget(parent), m_model(new QuickPhraseModel(this)),
      m_fileListModel(new FileListModel(this)) {
    setupUi(this);
    addButton->setText(_("&Add"));
    batchEditButton->setText(_("&Batch Edit"));
    deleteButton->setText(_("&Delete"));
    clearButton->setText(_("De&lete All"));
    importButton->setText(_("&Import"));
    exportButton->setText(_("E&xport"));
    operationButton->setText(_("&Operation"));
    macroTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    macroTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    macroTableView->setEditTriggers(QAbstractItemView::DoubleClicked);

    macroTableView->horizontalHeader()->setStretchLastSection(true);
    macroTableView->verticalHeader()->setVisible(false);
    macroTableView->setModel(m_model);
    fileListComboBox->setModel(m_fileListModel);

    m_operationMenu = new QMenu(this);
    m_operationMenu->addAction(_("Add File"), this,
                               &ListEditor::addFileTriggered);
    m_operationMenu->addAction(_("Remove File"), this,
                               &ListEditor::removeFileTriggered);
    m_operationMenu->addAction(_("Refresh List"), this,
                               &ListEditor::refreshListTriggered);
    operationButton->setMenu(m_operationMenu);

    loadFileList();
    itemFocusChanged();

    connect(addButton, &QPushButton::clicked, this, &ListEditor::addWord);
    connect(batchEditButton, &QPushButton::clicked, this,
            &ListEditor::batchEditWord);
    connect(deleteButton, &QPushButton::clicked, this, &ListEditor::deleteWord);
    connect(clearButton, &QPushButton::clicked, this,
            &ListEditor::deleteAllWord);
    connect(importButton, &QPushButton::clicked, this, &ListEditor::importData);
    connect(exportButton, &QPushButton::clicked, this, &ListEditor::exportData);

    connect(fileListComboBox, qOverload<int>(&QComboBox::activated), this,
            &ListEditor::changeFile);

    connect(macroTableView->selectionModel(),
            &QItemSelectionModel::selectionChanged, this,
            &ListEditor::itemFocusChanged);
    connect(m_model, &QuickPhraseModel::needSaveChanged, this,
            &ListEditor::changed);
}

ListEditor::~ListEditor() {}

void ListEditor::load() {
    m_lastFile = currentFile();
    m_model->load(currentFile(), false);
}

void ListEditor::load(const QString &file) { m_model->load(file, true); }

void ListEditor::save(const QString &file) { m_model->save(file); }

void ListEditor::save() {
    // QFutureWatcher< bool >* futureWatcher =
    // m_model->save("data/QuickPhrase.mb");
    QFutureWatcher<bool> *futureWatcher = m_model->save(currentFile());
    connect(futureWatcher, &QFutureWatcherBase::finished, this,
            &ListEditor::saveFinished);
}

QString ListEditor::addon() { return "fcitx-quickphrase"; }

bool ListEditor::asyncSave() { return true; }

void ListEditor::changeFile(int) {
    if (m_model->needSave()) {
        int ret = QMessageBox::question(
            this, _("Save Changes"),
            _("The content has changed.\n"
              "Do you want to save the changes or discard them?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            // save(fileListComboBox->itemText(lastFileIndex));
            save(m_lastFile);
        } else if (ret == QMessageBox::Cancel) {
            fileListComboBox->setCurrentIndex(
                m_fileListModel->findFile(m_lastFile));
            return;
        }
    }
    load();
}

QString ListEditor::title() { return _("Quick Phrase Editor"); }

void ListEditor::itemFocusChanged() {
    deleteButton->setEnabled(macroTableView->currentIndex().isValid());
}

void ListEditor::deleteWord() {
    if (!macroTableView->currentIndex().isValid())
        return;
    int row = macroTableView->currentIndex().row();
    m_model->deleteItem(row);
}

void ListEditor::deleteAllWord() { m_model->deleteAllItem(); }

void ListEditor::addWord() {
    EditorDialog *dialog = new EditorDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->open();
    connect(dialog, &QDialog::accepted, this, &ListEditor::addWordAccepted);
}

void ListEditor::batchEditWord() {
    BatchDialog *dialog = new BatchDialog(this);
    QString text;
    QTextStream stream(&text);
    m_model->saveData(stream);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setText(text);
    dialog->open();
    connect(dialog, &QDialog::accepted, this, &ListEditor::batchEditAccepted);
}

void ListEditor::addWordAccepted() {
    const EditorDialog *dialog =
        qobject_cast<const EditorDialog *>(QObject::sender());

    m_model->addItem(dialog->key(), dialog->value());
    QModelIndex last = m_model->index(m_model->rowCount() - 1, 0);
    macroTableView->setCurrentIndex(last);
    macroTableView->scrollTo(last);
}

void ListEditor::batchEditAccepted() {
    const BatchDialog *dialog =
        qobject_cast<const BatchDialog *>(QObject::sender());

    QString s = dialog->text();
    QTextStream stream(&s);

    m_model->loadData(stream);
    QModelIndex last = m_model->index(m_model->rowCount() - 1, 0);
    macroTableView->setCurrentIndex(last);
    macroTableView->scrollTo(last);
}

void ListEditor::importData() {
    QFileDialog *dialog = new QFileDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->setAcceptMode(QFileDialog::AcceptOpen);
    dialog->open();
    connect(dialog, &QDialog::accepted, this, &ListEditor::importFileSelected);
}

void ListEditor::exportData() {
    QFileDialog *dialog = new QFileDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setAcceptMode(QFileDialog::AcceptSave);
    dialog->open();
    connect(dialog, &QDialog::accepted, this, &ListEditor::exportFileSelected);
}

void ListEditor::importFileSelected() {
    const QFileDialog *dialog =
        qobject_cast<const QFileDialog *>(QObject::sender());
    if (dialog->selectedFiles().length() <= 0)
        return;
    QString file = dialog->selectedFiles()[0];
    load(file);
}

void ListEditor::exportFileSelected() {
    const QFileDialog *dialog =
        qobject_cast<const QFileDialog *>(QObject::sender());
    if (dialog->selectedFiles().length() <= 0)
        return;
    QString file = dialog->selectedFiles()[0];
    save(file);
}

void ListEditor::loadFileList() {
    int row = fileListComboBox->currentIndex();
    int col = fileListComboBox->modelColumn();
    QString lastFileName =
        m_fileListModel->data(m_fileListModel->index(row, col), Qt::UserRole)
            .toString();
    m_fileListModel->loadFileList();
    fileListComboBox->setCurrentIndex(m_fileListModel->findFile(lastFileName));
    load();
}

QString ListEditor::currentFile() {
    int row = fileListComboBox->currentIndex();
    int col = fileListComboBox->modelColumn();
    return m_fileListModel->data(m_fileListModel->index(row, col), Qt::UserRole)
        .toString();
}

QString ListEditor::currentName() {
    int row = fileListComboBox->currentIndex();
    int col = fileListComboBox->modelColumn();
    return m_fileListModel
        ->data(m_fileListModel->index(row, col), Qt::DisplayRole)
        .toString();
}

void ListEditor::addFileTriggered() {
    bool ok;
    QString filename = QInputDialog::getText(
        this, _("Create new file"), _("Please input a filename for newfile"),
        QLineEdit::Normal, "newfile", &ok);

    if (filename.contains('/')) {
        QMessageBox::warning(this, _("Invalid filename"),
                             _("File name should not contain '/'."));
        return;
    }

    filename.append(".mb");
    if (!StandardPath::global().safeSave(
            StandardPath::Type::PkgData,
            stringutils::joinPath(QUICK_PHRASE_CONFIG_DIR,
                                  filename.toLocal8Bit().constData()),
            [](int) { return true; })) {
        QMessageBox::warning(
            this, _("File Operation Failed"),
            QString(_("Cannot create file %1.")).arg(filename));
        return;
    }

    m_fileListModel->loadFileList();
    fileListComboBox->setCurrentIndex(m_fileListModel->findFile(
        filename.prepend(QUICK_PHRASE_CONFIG_DIR "/")));
    load();
}

void ListEditor::refreshListTriggered() { loadFileList(); }

void ListEditor::removeFileTriggered() {
    QString filename = currentFile();
    QString curName = currentName();
    auto fullname = stringutils::joinPath(
        StandardPath::global().userDirectory(StandardPath::Type::PkgData),
        filename.toLocal8Bit().constData());
    QFile f(fullname.data());
    if (!f.exists()) {
        int ret = QMessageBox::question(
            this, _("Cannot remove system file"),
            QString(_("%1 is a system file, do you want to delete all phrases "
                      "instead?"))
                .arg(curName),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (ret == QMessageBox::Yes) {
            deleteAllWord();
        }
        return;
    }

    int ret = QMessageBox::question(
        this, _("Confirm deletion"),
        QString(_("Are you sure to delete %1?")).arg(curName),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Ok) {
        bool ok = f.remove();
        if (!ok) {
            QMessageBox::warning(
                this, _("File Operation Failed"),
                QString(_("Error while deleting %1.")).arg(curName));
        }
    }
    loadFileList();
    load();
}
}
