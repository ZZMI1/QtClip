#ifndef QTCLIP_QCLEGACYFILTERSUPPORT_H_
#define QTCLIP_QCLEGACYFILTERSUPPORT_H_

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;

namespace QCLegacyFilterSupport
{
void initializeAndHideLegacyFilterControls(QLineEdit *pSearchLineEdit,
                                           QComboBox *pSearchHistoryComboBox,
                                           QPushButton *pClearSearchButton,
                                           QPushButton *pClearSearchHistoryButton,
                                           QPushButton *pResetFiltersButton,
                                           QComboBox *pSnippetTypeComboBox,
                                           QCheckBox *pShowArchivedCheckBox,
                                           QComboBox *pTagFilterComboBox);
}

#endif
