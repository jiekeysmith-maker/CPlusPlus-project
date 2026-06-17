#ifndef SOURCEDIALOG_H
#define SOURCEDIALOG_H

#include <QDialog>
#include <memory>
#include "manage.h"

class QLineEdit;
class QComboBox;
class QDoubleSpinBox;
class QStackedWidget;

class SourceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SourceDialog(QWidget *parent = nullptr);

    std::shared_ptr<Source> getSource() const;
    void setSource(const Source &src);
    void setForceType(const QString& type);

protected:
    void accept() override;

private slots:
    void onTypeChanged(int index);

private:
    void setupUi();

    QComboBox      *m_typeCombo;
    QLineEdit      *m_shortNameEdit;
    QLineEdit      *m_fullNameEdit;
    QLineEdit      *m_fieldEdit;
    QLineEdit      *m_publisherEdit;
    QLineEdit      *m_publisherAddrEdit;
    QLineEdit      *m_retrievalEdit;
    QLineEdit      *m_remarkEdit;
    QDoubleSpinBox *m_impactFactorSpin;
    QLineEdit      *m_meetingAddrEdit;
    QStackedWidget *m_typeStack;
};

#endif // SOURCEDIALOG_H
