/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "fcitxcandidatewindow.h"
#include "fcitxflags.h"
#include "fcitxtheme.h"
#include "qfcitxplatforminputcontext.h"
#include <QDebug>
#include <QExposeEvent>
#include <QFont>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPalette>
#include <QResizeEvent>
#include <QScreen>
#include <QTextLayout>
#include <QtMath>
#include <qpa/qplatformnativeinterface.h>
#include <utility>

namespace fcitx {

void doLayout(QTextLayout &layout) {
    QFontMetrics fontMetrics(layout.font());
    auto minH = fontMetrics.ascent() + fontMetrics.descent();
    layout.setCacheEnabled(true);
    layout.beginLayout();
    int height = 0;
    while (1) {
        QTextLine line = layout.createLine();
        if (!line.isValid())
            break;

        line.setLineWidth(INT_MAX);
        line.setLeadingIncluded(true);

        line.setPosition(
            QPoint(0, height - line.ascent() + fontMetrics.ascent()));
        height += minH;
    }
    layout.endLayout();
}

class MultilineText {
public:
    MultilineText(const QFont &font, const QString &text) {
        QStringList lines = text.split("\n");
        int currentY = 0;
        int width = 0;
        QFontMetrics fontMetrics(font);
        fontHeight_ = fontMetrics.ascent() + fontMetrics.descent();
        for (const auto &line : lines) {
            layouts_.emplace_back(std::make_unique<QTextLayout>(line));
            layouts_.back()->setFont(font);
            doLayout(*layouts_.back());
            width = std::max(width,
                             layouts_.back()->boundingRect().toRect().width());
            currentY += fontHeight_;
        }
        boundingRect_.setTopLeft(QPoint(0, 0));
        boundingRect_.setSize(QSize(width, lines.size() * fontHeight_));
    }

    bool isEmpty() const { return layouts_.empty(); }

    void draw(QPainter *painter, QColor color, QPoint position) {
        painter->save();
        painter->setPen(color);
        int currentY = 0;
        for (const auto &layout : layouts_) {
            layout->draw(painter, position + QPoint(0, currentY));
            currentY += fontHeight_;
        }
        painter->restore();
    }

    QRect boundingRect() { return boundingRect_; }

private:
    std::vector<std::unique_ptr<QTextLayout>> layouts_;
    int fontHeight_;
    QRect boundingRect_;
};

FcitxCandidateWindow::FcitxCandidateWindow(QWindow *window, FcitxTheme *theme)
    : QWindow(), theme_(theme), parent_(window) {
    setFlags(Qt::ToolTip | Qt::FramelessWindowHint |
             Qt::BypassWindowManagerHint | Qt::WindowDoesNotAcceptFocus |
             Qt::NoDropShadowWindowHint);
    if (isWayland_) {
        setTransientParent(parent_);
    }
    QSurfaceFormat surfaceFormat = format();
    surfaceFormat.setAlphaBufferSize(8);
    setFormat(surfaceFormat);
    backingStore_ = new QBackingStore(this);
}

FcitxCandidateWindow::~FcitxCandidateWindow() {}

bool FcitxCandidateWindow::event(QEvent *event) {
    if (event->type() == QEvent::UpdateRequest) {
        renderNow();
        return true;
    }
    if (event->type() == QEvent::Leave) {
        auto oldHighlight = highlight();
        hoverIndex_ = -1;
        if (highlight() != oldHighlight) {
            renderNow();
        }
    }
    return QWindow::event(event);
}

void FcitxCandidateWindow::renderLater() { requestUpdate(); }

void FcitxCandidateWindow::resizeEvent(QResizeEvent *) { renderNow(); }

void FcitxCandidateWindow::exposeEvent(QExposeEvent *) { renderNow(); }

void FcitxCandidateWindow::renderNow() {
    if (!isExposed() || !theme_) {
        return;
    }

    QRect rect(0, 0, width(), height());
    backingStore_->beginPaint(rect);

    QPaintDevice *device = backingStore_->paintDevice();
    QPainter painter(device);
    painter.fillRect(rect, Qt::transparent);
    render(&painter);
    painter.end();

    backingStore_->endPaint();
    backingStore_->flush(rect);
}

void FcitxCandidateWindow::render(QPainter *painter) {
    theme_->paint(painter, theme_->background(),
                  QRect(QPoint(0, 0), actualSize_));
    prevRegion_ = QRect();
    nextRegion_ = QRect();
    auto contentMargin = theme_->contentMargin();
    if (labelLayouts_.size() && (hasPrev_ || hasNext_)) {
        if (theme_->prev().valid() && theme_->next().valid()) {
            nextRegion_ =
                QRect(QPoint(actualSize_.width() - contentMargin.right() -
                                 theme_->prev().width(),
                             actualSize_.height() - contentMargin.bottom() -
                                 theme_->next().height()),
                      theme_->next().size());
            double alpha = 1.0;
            if (!hasNext_) {
                alpha = 0.3;
            } else if (nextHovered_) {
                alpha = 0.7;
            }
            theme_->paint(painter, theme_->next(), nextRegion_.topLeft(),
                          alpha);
            nextRegion_ = nextRegion_.marginsRemoved(theme_->next().margin());
            prevRegion_ = QRect(
                QPoint(actualSize_.width() - contentMargin.right() -
                           theme_->next().width() - theme_->prev().width(),
                       actualSize_.height() - contentMargin.bottom() -
                           theme_->prev().height()),
                theme_->prev().size());
            alpha = 1.0;
            if (!hasPrev_) {
                alpha = 0.3;
            } else if (prevHovered_) {
                alpha = 0.7;
            }
            theme_->paint(painter, theme_->prev(), prevRegion_.topLeft(),
                          alpha);
            prevRegion_ = prevRegion_.marginsRemoved(theme_->prev().margin());
        }
    }

    const QPoint topLeft(contentMargin.left(), contentMargin.top());
    painter->setPen(theme_->normalColor());
    auto minH =
        theme_->fontMetrics().ascent() + theme_->fontMetrics().descent();
    auto textMargin = theme_->textMargin();
    auto extraW = textMargin.left() + textMargin.right();
    auto extraH = textMargin.top() + textMargin.bottom();
    size_t currentHeight = 0;
    if (!upperLayout_.text().isEmpty()) {
        upperLayout_.draw(
            painter, topLeft + QPoint(textMargin.left(), textMargin.top()));
        // Draw cursor
        currentHeight += minH + extraH;
        if (cursor_ >= 0) {
            auto line = upperLayout_.lineForTextPosition(cursor_);
            if (line.isValid()) {
                int cursorX = line.cursorToX(cursor_);
                line.lineNumber();
                painter->save();
                QPen pen = painter->pen();
                pen.setWidth(2);
                painter->setPen(pen);
                QPoint start = topLeft + QPoint(textMargin.left() + cursorX + 1,
                                                textMargin.top() +
                                                    line.lineNumber() * minH);
                painter->drawLine(start, start + QPoint(0, minH));
                painter->restore();
            }
        }
    }
    if (!lowerLayout_.text().isEmpty()) {
        lowerLayout_.draw(painter,
                          topLeft + QPoint(textMargin.left(),
                                           textMargin.top() + currentHeight));
        currentHeight += minH + extraH;
    }

    bool vertical = theme_->vertical();
    if (layoutHint_ == FcitxCandidateLayoutHint::Vertical) {
        vertical = true;
    } else if (layoutHint_ == FcitxCandidateLayoutHint::Horizontal) {
        vertical = false;
    }

    candidateRegions_.clear();
    candidateRegions_.reserve(labelLayouts_.size());
    size_t wholeW = 0, wholeH = 0;

    // size of text = textMargin + actual text size.
    // HighLight = HighLight margin + TEXT.
    // Click region = HighLight - click

    for (size_t i = 0; i < labelLayouts_.size(); i++) {
        int x, y;
        if (vertical) {
            x = 0;
            y = currentHeight + wholeH;
        } else {
            x = wholeW;
            y = currentHeight;
        }
        x += textMargin.left();
        y += textMargin.top();
        int labelW = 0, labelH = 0, candidateW = 0, candidateH = 0;
        if (!labelLayouts_[i]->isEmpty()) {
            auto size = labelLayouts_[i]->boundingRect();
            labelW = size.width();
            labelH = size.height();
        }
        if (!candidateLayouts_[i]->isEmpty()) {
            auto size = candidateLayouts_[i]->boundingRect();
            candidateW = size.width();
            candidateH = size.height();
        }
        int vheight;
        if (vertical) {
            vheight = std::max({minH, labelH, candidateH});
            wholeH += vheight + extraH;
        } else {
            vheight = candidatesHeight_ - extraH;
            wholeW += candidateW + labelW + extraW;
        }
        QMargins highlightMargin = theme_->highlightMargin();
        QMargins clickMargin = theme_->highlightClickMargin();
        auto highlightWidth = labelW + candidateW;
        bool fullWidthHighlight = true;
        if (fullWidthHighlight && vertical) {
            // Last candidate, fill.
            highlightWidth = actualSize_.width() - contentMargin.left() -
                             contentMargin.right() - textMargin.left() -
                             textMargin.right();
        }
        const int highlightIndex = highlight();
        QColor color = theme_->normalColor();
        if (highlightIndex >= 0 && i == static_cast<size_t>(highlightIndex)) {
            // Paint highlight
            theme_->paint(
                painter, theme_->highlight(),
                QRect(topLeft + QPoint(x, y) -
                          QPoint(highlightMargin.left(), highlightMargin.top()),
                      QSize(highlightWidth + highlightMargin.left() +
                                highlightMargin.right(),
                            vheight + highlightMargin.top() +
                                highlightMargin.bottom())));
            color = theme_->highlightCandidateColor();
        }
        QRect candidateRegion(
            topLeft + QPoint(x, y) -
                QPoint(highlightMargin.left(), highlightMargin.right()) +
                QPoint(clickMargin.left(), clickMargin.right()),
            QSize(highlightWidth + highlightMargin.left() +
                      highlightMargin.right() - clickMargin.left() -
                      clickMargin.right(),
                  vheight + highlightMargin.top() + highlightMargin.bottom() -
                      clickMargin.top() - clickMargin.bottom()));
        candidateRegions_.push_back(candidateRegion);
        if (!labelLayouts_[i]->isEmpty()) {
            labelLayouts_[i]->draw(painter, color, topLeft + QPoint(x, y));
        }
        if (!candidateLayouts_[i]->isEmpty()) {
            candidateLayouts_[i]->draw(painter, color,
                                       topLeft + QPoint(x + labelW, y));
        }
    }
}

void UpdateLayout(QTextLayout &layout, const QFont &font,
                  std::initializer_list<
                      std::reference_wrapper<const FcitxQtFormattedPreeditList>>
                      texts) {
    layout.clearFormats();
    layout.setFont(font);
    QVector<QTextLayout::FormatRange> formats;
    QString str;
    int pos = 0;
    for (const auto &text : texts) {
        for (const auto &preedit : text.get()) {
            str += preedit.string();
            QTextCharFormat format;
            if (preedit.format() & FcitxTextFormatFlag_Underline) {
                format.setUnderlineStyle(QTextCharFormat::DashUnderline);
            }
            if (preedit.format() & FcitxTextFormatFlag_Strike) {
                format.setFontStrikeOut(true);
            }
            if (preedit.format() & FcitxTextFormatFlag_Bold) {
                format.setFontWeight(QFont::Bold);
            }
            if (preedit.format() & FcitxTextFormatFlag_Italic) {
                format.setFontItalic(true);
            }
            if (preedit.format() & FcitxTextFormatFlag_HighLight) {
                QBrush brush;
                QPalette palette;
                palette = QGuiApplication::palette();
                format.setBackground(QBrush(QColor(
                    palette.color(QPalette::Active, QPalette::Highlight))));
                format.setForeground(QBrush(QColor(palette.color(
                    QPalette::Active, QPalette::HighlightedText))));
            }
            formats.append(QTextLayout::FormatRange{
                pos, static_cast<int>(preedit.string().length()), format});
            pos += preedit.string().length();
        }
    }
    layout.setText(str);
    layout.setFormats(formats);
}

void FcitxCandidateWindow::updateClientSideUI(
    const FcitxQtFormattedPreeditList &preedit, int cursorpos,
    const FcitxQtFormattedPreeditList &auxUp,
    const FcitxQtFormattedPreeditList &auxDown,
    const FcitxQtStringKeyValueList &candidates, int candidateIndex,
    int layoutHint, bool hasPrev, bool hasNext) {
    bool preeditVisble = (cursorpos >= 0 || !preedit.isEmpty());
    bool auxUpVisbile = !auxUp.isEmpty();
    bool auxDownVisible = !auxDown.isEmpty();
    bool candidatesVisible = !candidates.isEmpty();
    bool visible =
        preeditVisble || auxUpVisbile || auxDownVisible || candidatesVisible;
    auto window = QGuiApplication::focusWindow();
    if (!theme_ || !visible || !QGuiApplication::focusWindow() ||
        window != parent_) {
        hide();
        hoverIndex_ = -1;
        return;
    }

    UpdateLayout(upperLayout_, theme_->font(), {auxUp, preedit});
    if (cursorpos >= 0) {
        int auxUpLength = 0;
        for (const auto &auxUpText : auxUp) {
            auxUpLength += auxUpText.string().length();
        }
        // Get the preedit part
        auto preeditString = upperLayout_.text().mid(auxUpLength).toUtf8();
        preeditString = preeditString.mid(0, cursorpos);
        cursor_ = auxUpLength + QString::fromUtf8(preeditString).length();
    } else {
        cursor_ = -1;
    }
    doLayout(upperLayout_);
    UpdateLayout(lowerLayout_, theme_->font(), {auxDown});
    doLayout(lowerLayout_);
    labelLayouts_.clear();
    candidateLayouts_.clear();
    for (int i = 0; i < candidates.size(); i++) {
        labelLayouts_.emplace_back(std::make_unique<MultilineText>(
            theme_->font(), candidates[i].key()));
        candidateLayouts_.emplace_back(std::make_unique<MultilineText>(
            theme_->font(), candidates[i].value()));
    }
    highlight_ = candidateIndex;
    hasPrev_ = hasPrev;
    hasNext_ = hasNext;
    layoutHint_ = static_cast<FcitxCandidateLayoutHint>(layoutHint);

    actualSize_ = sizeHint();

    QRect cursorRect =
        QGuiApplication::inputMethod()->cursorRectangle().toRect();
    QRect screenGeometry;
    // Try to apply the screen edge detection over the window, because if we
    // intent to use this with wayland. It we have no information above screen
    // edge.
    if (isWayland_) {
        screenGeometry = window->frameGeometry();
        cursorRect.translate(window->framePosition());
        auto margins = window->frameMargins();
        cursorRect.translate(margins.left(), margins.top());
    } else {
        screenGeometry = window->screen()->geometry();
        auto pos = window->mapToGlobal(cursorRect.topLeft());
        cursorRect.moveTo(pos);
    }

    int x = cursorRect.left(), y = cursorRect.bottom();
    if (cursorRect.left() + actualSize_.width() > screenGeometry.right()) {
        x = screenGeometry.right() - actualSize_.width() + 1;
    }

    if (x < screenGeometry.left()) {
        x = screenGeometry.left();
    }

    if (y + actualSize_.height() > screenGeometry.bottom()) {
        if (y > screenGeometry.bottom()) {
            y = screenGeometry.bottom() - actualSize_.height() - 40;
        } else { /* better position the window */
            y = y - actualSize_.height() -
                ((cursorRect.height() == 0) ? 40 : cursorRect.height());
        }
    }

    if (y < screenGeometry.top()) {
        y = screenGeometry.top();
    }

    backingStore_->resize(actualSize_);
    resize(actualSize_);
    // hide();
    QPoint newPosition(x, y);
    if (newPosition != position()) {
        if (isVisible()) {
            hide();
        }
        setPosition(newPosition);
    }
    renderNow();
    show();
}

void FcitxCandidateWindow::mouseMoveEvent(QMouseEvent *event) {
    bool needRepaint = false;
    auto oldHighlight = highlight();
    hoverIndex_ = -1;
    for (int idx = 0, e = candidateRegions_.size(); idx < e; idx++) {
        if (candidateRegions_[idx].contains(event->pos())) {
            hoverIndex_ = idx;
            break;
        }
    }

    needRepaint = needRepaint || oldHighlight != highlight();

    auto prevHovered = prevRegion_.contains(event->pos());
    auto nextHovered = nextRegion_.contains(event->pos());
    needRepaint = needRepaint || prevHovered_ != prevHovered;
    needRepaint = needRepaint || nextHovered_ != nextHovered;
    prevHovered_ = prevHovered;
    nextHovered_ = nextHovered;
    if (needRepaint) {
        renderNow();
    }
}

void FcitxCandidateWindow::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        return;
    }
    for (int idx = 0, e = candidateRegions_.size(); idx < e; idx++) {
        if (candidateRegions_[idx].contains(event->pos())) {
            Q_EMIT candidateSelected(idx);
            return;
        }
    }

    if (prevRegion_.contains(event->pos())) {
        Q_EMIT prevClicked();
        return;
    }

    if (nextRegion_.contains(event->pos())) {
        Q_EMIT nextClicked();
    }
}

QSize FcitxCandidateWindow::sizeHint() {
    auto minH =
        theme_->fontMetrics().ascent() + theme_->fontMetrics().descent();

    size_t width = 0;
    size_t height = 0;
    auto updateIfLarger = [](size_t &m, size_t n) {
        if (n > m) {
            m = n;
        }
    };
    auto textMargin = theme_->textMargin();
    auto extraW = textMargin.left() + textMargin.right();
    auto extraH = textMargin.top() + textMargin.bottom();
    if (!upperLayout_.text().isEmpty()) {
        auto size = upperLayout_.boundingRect();
        height += minH + extraH;
        updateIfLarger(width, size.width() + extraW);
    }
    if (!lowerLayout_.text().isEmpty()) {
        auto size = lowerLayout_.boundingRect();
        height += minH + extraH;
        updateIfLarger(width, size.width() + extraW);
    }

    bool vertical = theme_->vertical();
    if (layoutHint_ == FcitxCandidateLayoutHint::Vertical) {
        vertical = true;
    } else if (layoutHint_ == FcitxCandidateLayoutHint::Horizontal) {
        vertical = false;
    }

    size_t wholeH = 0, wholeW = 0;
    for (size_t i = 0; i < labelLayouts_.size(); i++) {
        size_t candidateW = 0, candidateH = 0;
        if (!labelLayouts_[i]->isEmpty()) {
            auto size = labelLayouts_[i]->boundingRect();
            candidateW += size.width();
            updateIfLarger(candidateH,
                           std::max(minH, qCeil(size.height())) + extraH);
        }
        if (!candidateLayouts_[i]->isEmpty()) {
            auto size = candidateLayouts_[i]->boundingRect();
            candidateW += size.width();
            updateIfLarger(candidateH,
                           std::max(minH, qCeil(size.height())) + extraH);
        }
        candidateW += extraW;

        if (vertical) {
            wholeH += candidateH;
            updateIfLarger(wholeW, candidateW);
        } else {
            wholeW += candidateW;
            updateIfLarger(wholeH, candidateH);
        }
    }
    updateIfLarger(width, wholeW);
    candidatesHeight_ = wholeH;
    height += wholeH;

    auto contentMargin = theme_->contentMargin();
    width += contentMargin.left() + contentMargin.right();
    height += contentMargin.top() + contentMargin.bottom();

    if (!labelLayouts_.empty() && (hasPrev_ || hasNext_)) {
        if (theme_->prev().valid() && theme_->next().valid()) {
            width += theme_->prev().width() + theme_->next().width();
        }
    }

    return {static_cast<int>(width), static_cast<int>(height)};
}

void FcitxCandidateWindow::wheelEvent(QWheelEvent *event) {
    if (!theme_ || !theme_->wheelForPaging()) {
        return;
    }
    accAngle_ += event->angleDelta().y();
    auto angleForClick = 120;
    while (accAngle_ >= angleForClick) {
        accAngle_ -= angleForClick;
        Q_EMIT prevClicked();
    }
    while (accAngle_ <= -angleForClick) {
        accAngle_ += angleForClick;
        Q_EMIT nextClicked();
    }
}

} // namespace fcitx
