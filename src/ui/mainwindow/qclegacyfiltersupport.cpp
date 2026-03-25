#include "qclegacyfiltersupport.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

namespace QCLegacyFilterSupport
{
void initializeAndHideLegacyFilterControls(QLineEdit *pSearchLineEdit,
                                           QComboBox *pSearchHistoryComboBox,
                                           QPushButton *pClearSearchButton,
                                           QPushButton *pClearSearchHistoryButton,
                                           QPushButton *pResetFiltersButton,
                                           QComboBox *pSnippetTypeComboBox,
                                           QCheckBox *pShowArchivedCheckBox,
                                           QComboBox *pTagFilterComboBox)
{
    if (nullptr != pSearchLineEdit)
    {
        pSearchLineEdit->setVisible(false);
        pSearchLineEdit->setClearButtonEnabled(true);
    }

    if (nullptr != pSearchHistoryComboBox)
    {
        pSearchHistoryComboBox->setVisible(false);
        pSearchHistoryComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
        pSearchHistoryComboBox->setMinimumWidth(170);
        pSearchHistoryComboBox->addItem(QCoreApplication::translate("QCLegacyFilterSupport", "Recent Searches"), QString());
    }

    if (nullptr != pClearSearchButton)
        pClearSearchButton->setVisible(false);
    if (nullptr != pClearSearchHistoryButton)
        pClearSearchHistoryButton->setVisible(false);
    if (nullptr != pResetFiltersButton)
        pResetFiltersButton->setVisible(false);

    if (nullptr != pSnippetTypeComboBox)
    {
        pSnippetTypeComboBox->setVisible(false);
        pSnippetTypeComboBox->addItem(QCoreApplication::translate("QCLegacyFilterSupport", "All Types"), QString::fromUtf8("all"));
        pSnippetTypeComboBox->addItem(QCoreApplication::translate("QCLegacyFilterSupport", "Text"), QString::fromUtf8("text"));
        pSnippetTypeComboBox->addItem(QCoreApplication::translate("QCLegacyFilterSupport", "Image"), QString::fromUtf8("image"));
    }

    if (nullptr != pShowArchivedCheckBox)
        pShowArchivedCheckBox->setVisible(false);

    if (nullptr != pTagFilterComboBox)
        pTagFilterComboBox->setVisible(false);
}
}
