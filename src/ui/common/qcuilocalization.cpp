// File: qcuilocalization.cpp
// Author: ZZMI1
// Created: 2026-03-25
// Description: Implements shared UI language helpers for bilingual text selection.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcuilocalization.h"

#include <QApplication>
#include <QHash>
#include <QRegularExpression>

namespace
{
bool IsBrokenChineseText(const QString& strText)
{
    const QString strTrimmed = strText.trimmed();
    if (strTrimmed.isEmpty())
        return true;

    if (strTrimmed.contains('?'))
        return true;

    static const QRegularExpression hanPattern(QString::fromUtf8("[\\x{4E00}-\\x{9FFF}]"));
    return !hanPattern.match(strTrimmed).hasMatch();
}

QString ChineseFallbackByEnglish(const QString& strEnglish)
{
    static const QHash<QString, QString> map = {
        {QString::fromUtf8("QtClip"), QString::fromUtf8("QtClip")},
        {QString::fromUtf8("File"), QString::fromUtf8("文件")},
        {QString::fromUtf8("Edit"), QString::fromUtf8("编辑")},
        {QString::fromUtf8("View"), QString::fromUtf8("视图")},
        {QString::fromUtf8("AI"), QString::fromUtf8("AI")},
        {QString::fromUtf8("Settings"), QString::fromUtf8("设置")},
        {QString::fromUtf8("Ready."), QString::fromUtf8("就绪。")},

        {QString::fromUtf8("Sessions"), QString::fromUtf8("Session 列表")},
        {QString::fromUtf8("Snippets"), QString::fromUtf8("Snippet 列表")},
        {QString::fromUtf8("Details"), QString::fromUtf8("详情")},
        {QString::fromUtf8("New"), QString::fromUtf8("新建")},
        {QString::fromUtf8("New Session"), QString::fromUtf8("新建 Session")},
        {QString::fromUtf8("Edit Session"), QString::fromUtf8("编辑 Session")},
        {QString::fromUtf8("Finish Session"), QString::fromUtf8("完成 Session")},
        {QString::fromUtf8("Reopen Session"), QString::fromUtf8("重新打开 Session")},
        {QString::fromUtf8("Delete Session"), QString::fromUtf8("删除 Session")},
        {QString::fromUtf8("Current Session"), QString::fromUtf8("当前 Session")},
        {QString::fromUtf8("Untitled Session"), QString::fromUtf8("未命名 Session")},
        {QString::fromUtf8("Active"), QString::fromUtf8("进行中")},
        {QString::fromUtf8("Finished"), QString::fromUtf8("已完成")},

        {QString::fromUtf8("New Text"), QString::fromUtf8("新建文本")},
        {QString::fromUtf8("New Text Snippet"), QString::fromUtf8("新建文本 Snippet")},
        {QString::fromUtf8("New Image Snippet"), QString::fromUtf8("新建图片 Snippet")},
        {QString::fromUtf8("Quick Capture"), QString::fromUtf8("快速记录")},
        {QString::fromUtf8("Manage Tags"), QString::fromUtf8("管理标签")},
        {QString::fromUtf8("Edit Snippet"), QString::fromUtf8("编辑 Snippet")},
        {QString::fromUtf8("Edit Text Snippet"), QString::fromUtf8("编辑文本 Snippet")},
        {QString::fromUtf8("Edit Image Snippet"), QString::fromUtf8("编辑图片 Snippet")},
        {QString::fromUtf8("Delete Snippet"), QString::fromUtf8("删除 Snippet")},
        {QString::fromUtf8("Duplicate Snippet"), QString::fromUtf8("复制 Snippet")},
        {QString::fromUtf8("Move Snippet"), QString::fromUtf8("移动 Snippet")},
        {QString::fromUtf8("Snippet Tags"), QString::fromUtf8("Snippet 标签")},
        {QString::fromUtf8("Clear Snippet Tags"), QString::fromUtf8("清空 Snippet 标签")},
        {QString::fromUtf8("Tag Library"), QString::fromUtf8("标签库")},
        {QString::fromUtf8("Archive / Restore"), QString::fromUtf8("归档 / 恢复")},
        {QString::fromUtf8("Archive Snippet"), QString::fromUtf8("归档 Snippet")},
        {QString::fromUtf8("Restore Snippet"), QString::fromUtf8("恢复 Snippet")},

        {QString::fromUtf8("Paste Image"), QString::fromUtf8("粘贴图片")},
        {QString::fromUtf8("Run Summary"), QString::fromUtf8("运行总结")},
        {QString::fromUtf8("Export"), QString::fromUtf8("导出")},
        {QString::fromUtf8("Export Markdown"), QString::fromUtf8("导出 Markdown")},

        {QString::fromUtf8("Search title, content, summary, note"), QString::fromUtf8("搜索标题、内容、总结、备注")},
        {QString::fromUtf8("Clear"), QString::fromUtf8("清空")},
        {QString::fromUtf8("Reset"), QString::fromUtf8("重置")},
        {QString::fromUtf8("History"), QString::fromUtf8("历史")},
        {QString::fromUtf8("Recent Searches"), QString::fromUtf8("最近搜索")},
        {QString::fromUtf8("All Types"), QString::fromUtf8("全部类型")},
        {QString::fromUtf8("Text"), QString::fromUtf8("文本")},
        {QString::fromUtf8("Image"), QString::fromUtf8("图片")},
        {QString::fromUtf8("All Tags"), QString::fromUtf8("全部标签")},
        {QString::fromUtf8("Selected Favorite"), QString::fromUtf8("选中项收藏")},
        {QString::fromUtf8("Selected Review"), QString::fromUtf8("选中项复习")},
        {QString::fromUtf8("Favorites Only"), QString::fromUtf8("仅收藏")},
        {QString::fromUtf8("Review Only"), QString::fromUtf8("仅复习")},
        {QString::fromUtf8("Show Archived"), QString::fromUtf8("显示已归档")},

        {QString::fromUtf8("Session Summary"), QString::fromUtf8("Session 总结")},
        {QString::fromUtf8("Snippet Summary"), QString::fromUtf8("Snippet 总结")},
        {QString::fromUtf8("View Session Summary"), QString::fromUtf8("查看 Session 总结")},
        {QString::fromUtf8("View Snippet Summary"), QString::fromUtf8("查看 Snippet 总结")},
        {QString::fromUtf8("Copy Session Summary"), QString::fromUtf8("复制 Session 总结")},
        {QString::fromUtf8("Copy Snippet Summary"), QString::fromUtf8("复制 Snippet 总结")},
        {QString::fromUtf8("Summarize Session"), QString::fromUtf8("总结 Session")},
        {QString::fromUtf8("Summarize Snippet"), QString::fromUtf8("总结 Snippet")},
        {QString::fromUtf8("Retry Session AI"), QString::fromUtf8("重试 Session AI")},
        {QString::fromUtf8("Retry Snippet AI"), QString::fromUtf8("重试 Snippet AI")},

        {QString::fromUtf8("Title"), QString::fromUtf8("标题")},
        {QString::fromUtf8("Tags"), QString::fromUtf8("标签")},
        {QString::fromUtf8("State"), QString::fromUtf8("状态")},
        {QString::fromUtf8("Favorite"), QString::fromUtf8("收藏")},
        {QString::fromUtf8("Review"), QString::fromUtf8("复习")},
        {QString::fromUtf8("Note"), QString::fromUtf8("备注")},
        {QString::fromUtf8("Content"), QString::fromUtf8("内容")},
        {QString::fromUtf8("Image Path"), QString::fromUtf8("图片路径")},
        {QString::fromUtf8("Preview"), QString::fromUtf8("预览")},

        {QString::fromUtf8("No image preview available."), QString::fromUtf8("暂无图片预览")},
        {QString::fromUtf8("No image attachment."), QString::fromUtf8("无图片附件")},
        {QString::fromUtf8("No tags."), QString::fromUtf8("无标签")},
        {QString::fromUtf8("No session selected."), QString::fromUtf8("未选择 Session")},
        {QString::fromUtf8("No session summary available."), QString::fromUtf8("暂无 Session 总结")},
        {QString::fromUtf8("Current context: no session selected."), QString::fromUtf8("当前上下文：未选择 Session")},
        {QString::fromUtf8("Current context: session '%1' [%2]. Session actions, export, and session AI are available."), QString::fromUtf8("当前上下文：Session '%1' [%2]。可执行 Session 操作、导出和 Session AI。")},
        {QString::fromUtf8("Current context: 1 selected snippet in '%1' [%2]. Single-item actions and session actions are available."), QString::fromUtf8("当前上下文：在 '%1' [%2] 中选中 1 条 snippet。可执行单项操作和 Session 操作。")},
        {QString::fromUtf8("Current context: %1 selected snippets in '%2' [%3]. Batch organize actions are available; edit and summarize stay single-snippet actions."), QString::fromUtf8("当前上下文：在 '%2' [%3] 中选中 %1 条 snippet。可执行批量整理；编辑和总结仍是单条操作。")},

        {QString::fromUtf8("Search cleared."), QString::fromUtf8("已清空搜索")},
        {QString::fromUtf8("Filters reset."), QString::fromUtf8("筛选已重置")},
        {QString::fromUtf8("Search history cleared."), QString::fromUtf8("搜索历史已清空")},
        {QString::fromUtf8("Session created."), QString::fromUtf8("Session 已创建")},
        {QString::fromUtf8("Session updated."), QString::fromUtf8("Session 已更新")},
        {QString::fromUtf8("Session finished."), QString::fromUtf8("Session 已完成")},
        {QString::fromUtf8("Session reopened."), QString::fromUtf8("Session 已重新打开")},
        {QString::fromUtf8("Session deleted."), QString::fromUtf8("Session 已删除")},
        {QString::fromUtf8("Snippet updated."), QString::fromUtf8("Snippet 已更新")},
        {QString::fromUtf8("Text snippet created."), QString::fromUtf8("文本 Snippet 已创建")},
        {QString::fromUtf8("Settings saved."), QString::fromUtf8("设置已保存")},

        {QString::fromUtf8("Capture Screen"), QString::fromUtf8("整屏截图")},
        {QString::fromUtf8("Capture Region"), QString::fromUtf8("区域截图")},
        {QString::fromUtf8("Import Image"), QString::fromUtf8("导入图片")},

        {QString::fromUtf8("Open File"), QString::fromUtf8("打开文件")},
        {QString::fromUtf8("Open Folder"), QString::fromUtf8("打开目录")},
        {QString::fromUtf8("Copy Path"), QString::fromUtf8("复制路径")},
        {QString::fromUtf8("Close"), QString::fromUtf8("关闭")},

        {QString::fromUtf8("OK"), QString::fromUtf8("确定")},
        {QString::fromUtf8("Cancel"), QString::fromUtf8("取消")},
        {QString::fromUtf8("Save"), QString::fromUtf8("保存")},
        {QString::fromUtf8("Delete"), QString::fromUtf8("删除")},
        {QString::fromUtf8("Warning"), QString::fromUtf8("警告")},
        {QString::fromUtf8("Error"), QString::fromUtf8("错误")},
        {QString::fromUtf8("Information"), QString::fromUtf8("提示")},
        {QString::fromUtf8("Question"), QString::fromUtf8("确认")},
        {QString::fromUtf8("Course"), QString::fromUtf8("课程")},
        {QString::fromUtf8("Description"), QString::fromUtf8("描述")},
        {QString::fromUtf8("Export Scope"), QString::fromUtf8("导出范围")},
        {QString::fromUtf8("Visible Snippets (%1)"), QString::fromUtf8("当前可见片段 (%1)")},
        {QString::fromUtf8("Selected Snippets (%1)"), QString::fromUtf8("选中片段 (%1)")},
        {QString::fromUtf8("completed"), QString::fromUtf8("已完成")},
        {QString::fromUtf8("failed"), QString::fromUtf8("失败")},
        {QString::fromUtf8("running"), QString::fromUtf8("正在运行")},
        {QString::fromUtf8("session"), QString::fromUtf8("会话")},
        {QString::fromUtf8("no session selected"), QString::fromUtf8("未选择会话")}
    };

    const QString strTrimmed = strEnglish.trimmed();
    return map.contains(strTrimmed) ? map.value(strTrimmed) : strEnglish;
}
}

QString QCNormalizeUiLanguage(const QString& strLanguage)
{
    return strLanguage.trimmed().compare(QString::fromUtf8("en-US"), Qt::CaseInsensitive) == 0
        ? QString::fromUtf8("en-US")
        : QString::fromUtf8("zh-CN");
}

bool QCIsChineseUiLanguage(const QString& strLanguage)
{
    return QCNormalizeUiLanguage(strLanguage) != QString::fromUtf8("en-US");
}

QString QCResolveUiLanguage()
{
    if (nullptr == qApp)
        return QString::fromUtf8("zh-CN");

    return QCNormalizeUiLanguage(qApp->property("qtclip.uiLanguage").toString());
}

bool QCIsChineseUi()
{
    return QCIsChineseUiLanguage(QCResolveUiLanguage());
}

QString QCUiText(const QString& strChinese, const QString& strEnglish)
{
    if (!QCIsChineseUi())
        return strEnglish;

    if (!IsBrokenChineseText(strChinese))
        return strChinese;

    return ChineseFallbackByEnglish(strEnglish);
}

