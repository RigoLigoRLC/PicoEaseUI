#ifndef COLOREDSTRINGLISTMODEL_H
#define COLOREDSTRINGLISTMODEL_H

#include <QStringListModel>
#include <QColor>
#include <QFont>

#ifdef _WIN32
constexpr const char *monofont = "Courier";
#else
constexpr const char *monofont = "Monospace";
#endif

// https://stackoverflow.com/a/37783141

class ColoredStringListModel : public QStringListModel
{
public:
    ColoredStringListModel(QObject* parent = nullptr) : QStringListModel(parent) {}

    QVariant data(const QModelIndex & index, int role) const override {
        if (role == Qt::ForegroundRole) {
            auto itr = m_rowColors.find(index.row());
            if (itr != m_rowColors.end())
                return itr->second;
        } else if (role == Qt::FontRole) {
            return QFont(monofont, 10);
        }

        return QStringListModel::data(index, role);
    }

    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override {
        if (role == Qt::ForegroundRole)
        {
            m_rowColors[index.row()] = value.value<QColor>();
            return true;
        }

        return QStringListModel::setData(index, value, role);
    }

    bool removeRows(int row, int count, const QModelIndex &parent) override {
        for (int i = row; i < row + count; i++) {
            m_rowColors.erase(i);
        }
        return QStringListModel::removeRows(row, count, parent);
    }
private:
    std::map<int, QColor> m_rowColors;
};

#endif // COLOREDSTRINGLISTMODEL_H
