# VBSQLite

VBSQLite 是一个基于 Kotlin Multiplatform 的跨平台 SQLite 数据库组件，支持 **Android**、**iOS**、**HarmonyOS（鸿蒙）** 三端统一 API。

demo：https://github.com/Kuikly-contrib/KuiklyLibGallery/blob/main/shared/src/commonMain/kotlin/com/example/kuiklylibgallery/SQLiteDemoPage.kt

---

# 一、快速开始

## 1.1 依赖接入

**Maven 仓库**：`https://mirrors.tencent.com/nexus/repository/maven-tencent`

### Android / iOS

在 `build.gradle.kts` 中添加：

```kotlin
implementation("com.tencent.kuiklybase:VBSQLite:1.0.2-2.0.21")
```

### 鸿蒙（HarmonyOS）

在 `build.ohos.gradle.kts` 中添加：

```kotlin
implementation("com.tencent.kuiklybase:VBSQLite:1.0.2-2.0.21-KBA-010")
```

## 1.2 各平台额外配置

### Android

**无需额外配置**。Android 系统自带 SQLite，VBSQLite 会自动使用系统的 SQLite 库。

### iOS

**无需额外配置**。iOS 系统自带 `libsqlite3.tbd`，VBSQLite 的 klib 通过 cinterop 机制在编译时自动链接系统 SQLite 库。

### HarmonyOS (OHOS)

鸿蒙端需要额外的配置步骤：

#### 1）放置 `libsqlite3.so`

`libsqlite3.so` 文件位于本组件源码的 [`VBSQLite/src/libs/libsqlite3.so`](VBSQLite/src/libs/libsqlite3.so) 目录下，将其复制到鸿蒙工程的 native 库目录：

```
ohosApp/entry/libs/arm64-v8a/libsqlite3.so
```

#### 2）配置 CMakeLists.txt

在 `ohosApp/entry/src/main/cpp/CMakeLists.txt` 中添加 SQLite 库的链接配置：

```cmake
# sqlite3 - SQLite 数据库库（VBSQLite 依赖）
add_library(sqlite3 SHARED IMPORTED)
set_target_properties(sqlite3
        PROPERTIES
        IMPORTED_LOCATION ${NATIVERENDER_ROOT_PATH}/../../../libs/${OHOS_ARCH}/libsqlite3.so)

# 在 target_link_libraries 中添加 sqlite3
target_link_libraries(entry PUBLIC libace_napi.z.so libhilog_ndk.z.so kuikly_shared kuikly_render sqlite3)
```

#### 3）提供数据库目录路径

鸿蒙端需要在页面加载时提供数据库目录路径，将应用沙盒的 `databaseDir` 注入到 `pageData` 中，Kuikly 页面获取后传递参数给VBSQLite初始化：

```typescript
// ohosApp/entry/src/main/ets/pages/Index.ets

@Entry
@Component
struct Index {
  // ...

  aboutToAppear() {
    this.injectDatabaseDir();
  }

  private injectDatabaseDir(): void {
    const ctx = getContext(this);
    if (!ctx || !ctx.databaseDir) {
      return;
    }
    if (!this.pageData) {
      this.pageData = JSON.parse('{}') as KRRecord;
    }
    const pd = this.pageData as Record<string, Object>;
    pd['databaseDir'] = ctx.databaseDir;
  }
}
```

## 1.3 初始化

使用前推荐先初始化日志模块：

```kotlin
import com.tencent.kmm.component.VBSQLite.SqliteInitTask
import com.tencent.kmm.component.VBSQLite.ILog

val logger = object : ILog {
    override fun d(tag: String, content: String) {
        println("[$tag] D: $content")
    }
    override fun i(tag: String, content: String) {
        println("[$tag] I: $content")
    }
    override fun e(tag: String, content: String, throwable: Throwable) {
        println("[$tag] E: $content, ${throwable.message}")
    }
}

SqliteInitTask.init(logger)
```

## 1.4 快速使用示例

以下是一个最小化的完整使用流程：

```kotlin
import com.tencent.kmm.component.VBSQLite.*

// 1. 创建数据库配置
val configuration = DefaultConfiguration(
    name = "my_app.db",
    path = databasePath,  // 平台相关，见 1.5 节
    version = 1,
    create = { connection ->
        connection.exec("""
            CREATE TABLE IF NOT EXISTS user (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                age INTEGER NOT NULL
            )
        """)
    }
)

// 2. 获取连接
val connection: IConnection = VBDatabaseConnection(configuration)

// 3. 插入数据
connection.exec(
    "INSERT INTO user (name, age) VALUES (?, ?)",
    arrayOf("张三", 25)
)

// 4. 查询数据
val cursor = connection.query("SELECT name, age FROM user")
try {
    while (cursor.next()) {
        val name = cursor.getString(0)
        val age = cursor.getInt(1)
        println("用户: $name, $age")
    }
} finally {
    cursor.close()
}

// 5. 关闭连接
connection.close()
```

## 1.5 平台路径获取方式

| 平台 | 获取方式 |
|------|---------|
| **Android** | `val databasePath = context.toDatabasePath()` |
| **iOS / HarmonyOS** | `val databasePath = "/your/database/directory".toDatabasePath()` |

---

# 二、API 文档

## 2.1 创建数据库配置

通过 `DefaultConfiguration` 创建数据库配置：

```kotlin
val configuration = DefaultConfiguration(
    name = "my_app.db",           // 数据库文件名
    path = databasePath,           // 数据库路径（平台相关）
    version = 1,                   // 数据库版本号
    journalMode = JournalMode.WAL, // 日志模式，默认 WAL
    synchronousMode = SynchronousMode.NORMAL, // 同步模式，默认 NORMAL
    busyTimeout = 3000,            // 忙等超时（毫秒），默认 3000
    create = { connection ->
        // 首次创建数据库时执行，在此建表
        connection.exec("""
            CREATE TABLE IF NOT EXISTS user (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                age INTEGER NOT NULL,
                email TEXT
            )
        """)
    },
    upgrade = { connection, oldVersion, newVersion ->
        // 数据库版本升级时执行
        if (oldVersion < 2) {
            connection.exec("ALTER TABLE user ADD COLUMN phone TEXT")
        }
    }
)
```

### 配置参数说明

#### JournalMode（日志模式）

| 值 | 说明 |
|----|------|
| `JournalMode.WAL` | Write-Ahead Logging，**默认推荐**，读写并发性能好 |
| `JournalMode.DELETE` | 回滚日志删除模式，兼容性好 |

#### SynchronousMode（同步模式）

| 值 | 说明 |
|----|------|
| `SynchronousMode.OFF` | 不同步，最快但断电可能丢数据 |
| `SynchronousMode.NORMAL` | 正常同步，**默认推荐**，性能与安全兼顾 |
| `SynchronousMode.FULL` | 完全同步，最安全但较慢 |
| `SynchronousMode.EXTRA` | 额外同步 |

## 2.2 获取数据库连接

```kotlin
val connection: IConnection = VBDatabaseConnection(configuration)
```

> 内部自动管理连接池，相同路径的配置会复用同一个连接。

## 2.3 执行 SQL（增 / 删 / 改 / 建表）

```kotlin
// 无参数
connection.exec("CREATE TABLE IF NOT EXISTS config (key TEXT PRIMARY KEY, value TEXT)")

// 带参数绑定（防 SQL 注入）
connection.exec(
    "INSERT INTO user (name, age, email) VALUES (?, ?, ?)",
    arrayOf("张三", 25, "zhangsan@example.com")
)

// 更新
connection.exec(
    "UPDATE user SET age = ? WHERE name = ?",
    arrayOf(26, "张三")
)

// 删除
connection.exec(
    "DELETE FROM user WHERE id = ?",
    arrayOf(1)
)
```

### exec 参数绑定支持的类型

| 类型 | 绑定方式 |
|------|---------|
| `String` | bindString |
| `Int`, `Long`, `Short`, `Byte` | bindLong |
| `Float`, `Double` | bindDouble |
| `ByteArray` | bindBlob |
| `Boolean` | bindLong（true=1, false=0） |
| `Char` | bindString |
| `ULong`, `UInt`, `UShort`, `UByte` | bindLong |
| `null` | bindNull |

## 2.4 查询数据

```kotlin
val cursor: ICursor = connection.query(
    "SELECT id, name, age, email FROM user WHERE age > ?",
    arrayOf("18")  // 注意：query 的 bindParams 是 String 数组
)

try {
    while (cursor.next()) {
        val id = cursor.getInt(0)
        val name = cursor.getString(1)
        val age = cursor.getInt(2)
        val email = cursor.getString(3)
        println("用户: id=$id, name=$name, age=$age, email=$email")
    }
} finally {
    cursor.close() // 务必关闭游标
}
```

### 通过列名获取数据

```kotlin
val cursor = connection.query("SELECT * FROM user")
try {
    val nameIndex = cursor.getColumnIndex("name")
    val ageIndex = cursor.getColumnIndex("age")
    while (cursor.next()) {
        val name = cursor.getString(nameIndex)
        val age = cursor.getInt(ageIndex)
        println("$name, $age")
    }
} finally {
    cursor.close()
}
```

### 遍历所有行（each）

```kotlin
val cursor = connection.query("SELECT name, age FROM user")
try {
    cursor.each { rowIndex ->
        val name = cursor.getString(0)
        val age = cursor.getInt(1)
        println("第${rowIndex}行: $name, $age")
    }
} finally {
    cursor.close()
}
```

### ICursor 支持的方法

| 方法 | 返回类型 | 说明 |
|------|---------|------|
| `getString(columnIndex)` | `String?` | 获取字符串，可能为 null |
| `getInt(columnIndex)` | `Int` | 获取整数 |
| `getLong(columnIndex)` | `Long` | 获取长整数 |
| `getFloat(columnIndex)` | `Float` | 获取浮点数 |
| `getDouble(columnIndex)` | `Double` | 获取双精度浮点数 |
| `getBlob(columnIndex)` | `ByteArray?` | 获取二进制数据 |
| `isNull(columnIndex)` | `Boolean` | 判断是否为 NULL |
| `getColumnIndex(name)` | `Int` | 通过列名获取列索引 |
| `getColumnName(index)` | `String?` | 通过索引获取列名 |
| `getColumnCount()` | `Int` | 获取列数 |
| `getColumnType(index)` | `Int` | 获取列的数据类型 |

## 2.5 事务操作

```kotlin
connection.begin { conn ->
    conn.exec("INSERT INTO user (name, age) VALUES (?, ?)", arrayOf("李四", 30))
    conn.exec("INSERT INTO user (name, age) VALUES (?, ?)", arrayOf("王五", 28))

    // 返回 STMT.COMMIT 提交事务
    // 返回 STMT.ROLLBACK 回滚事务
    STMT.COMMIT
}
```

异常自动回滚示例：

```kotlin
connection.begin { conn ->
    conn.exec("INSERT INTO user (name, age) VALUES (?, ?)", arrayOf("赵六", 35))

    if (someConditionFailed) {
        return@begin STMT.ROLLBACK
    }

    STMT.COMMIT
}
```

> **注意**：事务支持嵌套，但建议尽量避免。子事务中的 ROLLBACK 会回滚所有操作（包括父事务）。

## 2.6 删除数据库

```kotlin
// Android
val deleted = deleteDatabase(context.toDatabasePath(), "my_app.db")

// iOS / HarmonyOS（当前返回 false，需手动删除文件）
```

## 2.7 关闭连接

```kotlin
connection.close()
```

---

## 附录 A：iOS 原生调用示例

```objective-c
#import <VBSQLite/VBSQLite.h>

// 打开数据库
id<VBSQLIConnection> dbConnection = [VBSQLVBDatabase_iosKt
    VBDatabaseConnectionConfiguration:({
    [[VBSQLDefaultConfiguration alloc]
        initWithName:DB_FILENAME
        path:dbPath
        logger:self
        version:1
        journalMode:VBSQLJournalMode.wal
        synchronousMode:VBSQLSynchronousMode.normal
        busyTimeout:5000
        create:^(id<VBSQLIConnection> _Nonnull conn) {
            // 建表
        }
        upgrade:^(id<VBSQLIConnection> _Nonnull conn,
                  VBSQLInt * _Nonnull oldVersion,
                  VBSQLInt * _Nonnull newVersion) {
            // 升级
        }];
})];

// 查询
id<VBSQLICursor> cursor = [dbConnection querySQL:sql bindParams:nil];
[cursor eachBlock:^(VBSQLInt * _Nonnull row) {
    // 遍历每行数据
}];
```

## 附录 B：Android Java 调用示例

```java
// 打开数据库
IConfiguration config = new DefaultConfiguration(
    "test.db",
    context.toDatabasePath(),
    logger
);
IConnection connection = VBDatabaseConnection(config);

// 事务操作
connection.begin(new Function1<IConnection, STMT>() {
    @Override
    public STMT invoke(IConnection conn) {
        ICursor cursor = conn.query("SELECT * FROM table", null);
        cursor.each(new Function1<Integer, Unit>() {
            @Override
            public Unit invoke(Integer row) {
                // 处理每行数据
                return null;
            }
        });
        return STMT.COMMIT;
    }
});
```

---

