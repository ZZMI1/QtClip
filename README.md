# QtClip

## 项目简介

QtClip 是一个基于 Qt5 Widgets 的桌面学习工具，当前目标是实现“网课截图整理与 AI 笔记助手”的首版闭环。

它面向的核心使用场景是：在上课、看录播或自学过程中，把零散的截图、导入图片、文字记录整理成可检索、可总结、可导出的学习片段，并进一步沉淀为结构化学习笔记。

当前主链路已经打通：

> 上课学习 -> 截图或导入 -> 快速记录 -> 保存为 snippet -> AI 总结 -> 标签整理 -> 搜索筛选 -> Markdown 导出

QtClip 目前处于“首版可用，持续迭代中”的阶段，重点是把现有功能做稳定、做顺手，而不是一次性扩成完整的大型产品。

---

## 当前功能

### 已完成能力

- Study Session 创建、结束、浏览
- Text Snippet 创建
- Image Snippet + Primary Attachment 创建
- 整屏截图
- 区域截图
- QuickCapture 快速记录
- AI Settings 设置入口
- Mock Provider 可跑通
- OpenAI-compatible Provider 已实现
- AI 调用异步化，不阻塞 UI
- `Summarize Snippet` / `Summarize Session` 已接入 UI
- Image Snippet 保存后可按设置自动触发异步总结
- `snippet.summary` 写回
- `session summary` 通过 `ai_records` 使用
- Tag 持久化
- Snippet Tag 绑定与编辑
- Markdown 导出真实生成
- 最小搜索与过滤
- Favorite 状态编辑
- Review 状态编辑
- 清空搜索
- 恢复默认筛选
- Settings 首版可用
- `screenshot save directory` 已接入 `Capture Screen / Capture Region`
- `export directory` 已接入 Markdown 导出
- Import Image 已支持可选复制到默认截图目录
- 编译通过
- GUI 启动通过
- Smoke 通过

### 当前更适合怎么理解

QtClip 现在已经不是“原型草图”，而是一个可以演示、可以自用、主链路可闭环的桌面应用首版。

---

## 当前界面 / 使用流程概览

### 日常使用流程

1. 创建一个 Study Session
2. 通过以下任一方式采集学习内容
   - 新建文本 Snippet
   - 整屏截图
   - 区域截图
   - 导入已有图片
3. 在 QuickCapture 或创建对话框中补充标题、备注等信息
4. 保存后形成 Snippet
5. 根据设置决定是否自动触发 AI 总结
6. 给 Snippet 绑定 Tag，或标记 Favorite / Review
7. 通过搜索、Tag、Favorite、Review 进行筛选
8. 需要归档时导出 Markdown

### 主界面当前包含的核心区域

- Session 列表
- Snippet 列表
- 搜索与基础筛选区域
- Snippet 详情区域
- 工具栏操作入口
- Settings 设置入口

### 截图占位说明

后续建议在 README 中补充真实界面截图，例如：

- 主窗口截图
- QuickCapture 对话框截图
- Settings 截图
- Capture Region 覆盖层截图

当前可以先保留本 README 结构，后续直接替换为真实图片。

---

## 技术栈

- Qt 5.15.x
- Qt Widgets
- C++17
- Visual Studio 2022
- CMake
- SQLite

### 架构约束

- 不使用 QML
- UI 不直接访问数据库
- 所有 SQL 只在 `repository/sqlite` 层
- `service` 保持薄层
- `domain` 保持纯数据对象

---

## 项目结构说明

当前源码主要位于 `src/` 下，按分层组织：

```text
src/
├─ app/                  # 程序入口
├─ core/
│  └─ database/          # 数据库基础设施、schema、enum codec
├─ domain/
│  ├─ entities/          # 纯数据对象
│  └─ interfaces/        # repository / provider 接口定义
├─ repository/
│  └─ sqlite/            # SQLite 持久化实现，所有 SQL 集中在这里
├─ services/             # 薄服务层，负责编排与最小校验
└─ ui/
   ├─ mainwindow/        # 主窗口
   ├─ dialogs/           # 各种最小对话框
   └─ widgets/           # 覆盖层等自定义小部件
```

### 当前分层职责

#### `domain`
- 只放实体和接口
- 不放 UI、数据库、网络逻辑

#### `repository/sqlite`
- 负责 SQLite 持久化实现
- 所有 SQL 都在这一层

#### `services`
- 保持薄层
- 负责最小校验、调用 repository、错误透传
- AI 处理服务负责编排 provider 调用与 AI record 写回
- 导出服务负责导出上下文组装、Markdown 渲染、文件写出
- 截图服务负责整屏 / 区域截图及导入图片相关文件处理

#### `ui`
- 基于 Qt Widgets
- 只通过 service 层工作
- 不直接访问数据库或执行 SQL

---

## 构建与运行

### 环境要求

建议在 Windows 环境下构建，当前以以下组合为主：

- Windows
- Visual Studio 2022
- CMake
- Qt 5.15.x
- 可用的 Qt5 Widgets / Network / Sql / Concurrent 组件

### 构建方式

在项目根目录执行：

```powershell
cmake -S . -B out
cmake --build out --config Release
```

### 运行方式

构建完成后，可执行文件位于：

```text
out/Release/qtclipsmoke.exe
```

直接启动 GUI：

```powershell
out/Release/qtclipsmoke.exe
```

运行最小 smoke 验证：

```powershell
out/Release/qtclipsmoke.exe --smoke
```

### 当前已验证状态

- Release 构建通过
- GUI 启动通过
- Smoke 通过

---

## AI 配置说明

QtClip 当前支持两类 AI provider 使用方式。

### 1. Mock Provider

适合：

- 离线演示
- 不依赖真实网络环境的功能验证
- 本地 UI / 流程联调

特点：

- 不需要真实 API 配置
- 可以跑通 `Summarize Snippet` / `Summarize Session`
- 可以验证 AI 异步链路、写回逻辑和 UI 刷新逻辑

### 2. 真实 OpenAI-compatible Provider

适合：

- 接入兼容 OpenAI Chat Completions 风格接口的真实服务
- 做真实模型摘要测试

需要用户自行提供并配置：

- `baseUrl`
- `apiKey`
- `model`

### 当前 Settings 中可配置项

- 是否使用 Mock Provider
- 是否自动总结 Image Snippet
- AI Base URL
- AI API Key
- AI Model
- Screenshot Save Directory
- Markdown Export Directory
- Import Image 默认复制行为

### Test AI Connection

Settings 中已提供最小可用的 `Test AI Connection` 按钮，特点是：

- 异步执行，不阻塞 UI
- 可直接测试当前对话框中正在编辑的 AI 配置
- 成功时会显示：
  - 当前 provider mode
  - 当前测试 model
  - 实际请求 endpoint
- 失败时会尽量显示：
  - endpoint
  - HTTP status（如果有）
  - provider 错误摘要（如果有）
  - 更可读的错误信息

### 使用建议

建议先这样验证真实 AI：

1. 打开 Settings
2. 填写真实 `baseUrl / apiKey / model`
3. 先点击 `Test AI Connection`
4. 测试通过后，再去主界面手动执行一次 `Summarize Snippet`

---

## 当前完成度与已知限制

### 当前完成度判断

QtClip 当前已经具备首版桌面应用形态，主链路已闭环：

> Session -> Snippet -> Screenshot / Import Image -> AI Summary -> Tag -> Search / Filter -> Markdown Export

这意味着它已经适合：

- 功能演示
- 自己日常试用
- 持续迭代开发

### 当前已知限制

- 真实 AI provider 在线联调仍需要在本机使用真实配置完成
- 快捷键 / 全局热键未做
- 统计页未做
- ExportRepository 尚未落库
- 完整 Tag 管理中心未做
- 高级筛选面板未做
- 搜索历史、关键字高亮未做
- 多附件系统未扩展
- 更完整的测试体系仍需继续补
- 当前仍以“首版可用”优先，不是完整成熟产品

---

## Roadmap / 后续计划

后续迭代会继续围绕“现有主链路产品化增强”展开，优先级大致如下：

### 近期方向

- 继续完成真实 AI provider 的本机联调收尾
- 提升已有流程的稳定性与错误可定位性
- 继续补强最小人工验证和自动 smoke 覆盖

### 中期方向

- 更完整的 Tag 管理能力
- 更完善的搜索与筛选体验
- 更清楚的列表整理动作
- 更稳定的 AI 交互与失败处理

### 暂不在当前优先级的方向

- QML 重构
- 大规模架构重写
- 复杂设置中心
- 快捷键 / 全局热键
- 统计页
- 多附件系统扩张
- 复杂资源管理系统

---

## 仓库状态说明

这个仓库当前对应的是 **持续开发中的首版可用项目**。

它已经具备：

- 可构建
- 可启动
- 可 smoke
- 主链路可运行

但仍处于持续迭代阶段，后续会继续围绕稳定性、易用性和真实联调体验做小步推进。

如果你现在查看这个仓库，可以把它理解为：

> 一个已经完成首版核心闭环、正在继续打磨的 Qt Widgets 桌面学习工具。
