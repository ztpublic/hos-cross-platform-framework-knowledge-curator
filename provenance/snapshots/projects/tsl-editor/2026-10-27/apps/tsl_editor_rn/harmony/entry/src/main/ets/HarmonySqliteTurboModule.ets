// 鸿蒙原生 SQLite TurboModule：用 @ohos.data.relationalStore 实现 watermelondb 需要的
// Execute/SQL 能力，注册为 RN 的 'HarmonySqlite' 模块（UITurboModule，免 codegen）。
// 注意：relationalStore 不支持 PRAGMA 读写，schema 版本用独立 wm_meta 表管理。
import { UITurboModule } from '@rnoh/react-native-openharmony/ts';
import relationalStore from '@ohos.data.relationalStore';

const TAG = '[shared-core/db]';
const CREATE_META =
  "CREATE TABLE IF NOT EXISTS wm_meta (key TEXT PRIMARY KEY, value TEXT)";

function logError(method: string, e: Error): void {
  console.log(`${TAG} [HarmonySqlite] ${method} FAILED:`, e?.message ?? String(e), e);
}

export class HarmonySqliteTurboModule extends UITurboModule {
  static readonly NAME = 'HarmonySqlite';

  private store: relationalStore.RdbStore | null = null;

  async open(dbName: string): Promise<void> {
    console.log(`${TAG} [HarmonySqlite] open(${dbName})`);
    try {
      if (this.store) {
        return;
      }
      const config: relationalStore.StoreConfig = {
        name: dbName,
        securityLevel: relationalStore.SecurityLevel.S1,
      };
      this.store = await relationalStore.getRdbStore(this.ctx.uiAbilityContext, config);
      await this.store.executeSql(CREATE_META, []);
      console.log(`${TAG} [HarmonySqlite] open done`);
    } catch (e) {
      logError('open', e as Error);
      throw new Error(`[HarmonySqlite] open failed: ${String(e)}`);
    }
  }

  async executeSql(sql: string, args: Array<relationalStore.ValueType>): Promise<void> {
    console.log(`${TAG} [HarmonySqlite] executeSql: ${sql.slice(0, 80)}`);
    try {
      await this.requireStore().executeSql(sql, args ?? []);
    } catch (e) {
      logError('executeSql', e as Error);
      throw new Error(`[HarmonySqlite] executeSql failed: ${String(e)}`);
    }
  }

  async query(
    sql: string,
    args: Array<relationalStore.ValueType>,
  ): Promise<Array<Record<string, relationalStore.ValueType>>> {
    console.log(`${TAG} [HarmonySqlite] query: ${sql.slice(0, 80)}`);
    try {
      const resultSet = await this.requireStore().querySql(sql, args ?? []);
      const rows: Array<Record<string, relationalStore.ValueType>> = [];
      while (resultSet.goToNextRow()) {
        const row: Record<string, relationalStore.ValueType> = {};
        for (const col of resultSet.columnNames) {
          row[col] = resultSet.getValue(resultSet.getColumnIndex(col));
        }
        rows.push(row);
      }
      resultSet.close();
      console.log(`${TAG} [HarmonySqlite] query done, rows=${rows.length}`);
      return rows;
    } catch (e) {
      logError('query', e as Error);
      throw new Error(`[HarmonySqlite] query failed: ${String(e)}`);
    }
  }

  async executeStatements(sqls: string): Promise<void> {
    console.log(`${TAG} [HarmonySqlite] executeStatements len=${sqls.length}`);
    try {
      const statements = sqls.split(';').map((s) => s.trim()).filter(Boolean);
      for (const stmt of statements) {
        await this.executeSql(stmt, []);
      }
    } catch (e) {
      logError('executeStatements', e as Error);
      throw new Error(`[HarmonySqlite] executeStatements failed: ${String(e)}`);
    }
  }

  async userVersion(): Promise<number> {
    console.log(`${TAG} [HarmonySqlite] userVersion`);
    try {
      const rows = await this.query("SELECT value FROM wm_meta WHERE key='user_version'", []);
      const v = rows.length === 0 ? 0 : Number(rows[0]['value'] ?? 0);
      console.log(`${TAG} [HarmonySqlite] userVersion = ${v}`);
      return v;
    } catch (e) {
      logError('userVersion', e as Error);
      throw new Error(`[HarmonySqlite] userVersion failed: ${String(e)}`);
    }
  }

  async setUserVersion(version: number): Promise<void> {
    console.log(`${TAG} [HarmonySqlite] setUserVersion(${version})`);
    try {
      await this.requireStore().executeSql(CREATE_META, []);
      await this.executeSql("INSERT OR REPLACE INTO wm_meta(key, value) VALUES('user_version', ?)", [
        String(Number(version)),
      ]);
    } catch (e) {
      logError('setUserVersion', e as Error);
      throw new Error(`[HarmonySqlite] setUserVersion failed: ${String(e)}`);
    }
  }

  async unsafeDestroyEverything(): Promise<void> {
    console.log(`${TAG} [HarmonySqlite] unsafeDestroyEverything`);
    try {
      const rows = await this.query(
        "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' AND name != 'wm_meta'",
        [],
      );
      for (const r of rows) {
        await this.executeSql(`DROP TABLE IF EXISTS "${r.name}"`, []);
      }
      await this.executeSql("INSERT OR REPLACE INTO wm_meta(key, value) VALUES('user_version', '0')", []);
      console.log(`${TAG} [HarmonySqlite] unsafeDestroyEverything done`);
    } catch (e) {
      logError('unsafeDestroyEverything', e as Error);
      throw new Error(`[HarmonySqlite] unsafeDestroyEverything failed: ${String(e)}`);
    }
  }

  async beginTransaction(): Promise<void> {
    console.log(`${TAG} [HarmonySqlite] beginTransaction`);
    try {
      this.requireStore().beginTransaction();
    } catch (e) {
      logError('beginTransaction', e as Error);
      throw new Error(`[HarmonySqlite] beginTransaction failed: ${String(e)}`);
    }
  }

  async commit(): Promise<void> {
    console.log(`${TAG} [HarmonySqlite] commit`);
    try {
      this.requireStore().commit();
    } catch (e) {
      logError('commit', e as Error);
      throw new Error(`[HarmonySqlite] commit failed: ${String(e)}`);
    }
  }

  async rollBack(): Promise<void> {
    console.log(`${TAG} [HarmonySqlite] rollBack`);
    try {
      this.requireStore().rollBack();
    } catch (e) {
      logError('rollBack', e as Error);
      throw new Error(`[HarmonySqlite] rollBack failed: ${String(e)}`);
    }
  }

  private requireStore(): relationalStore.RdbStore {
    if (!this.store) {
      throw new Error('[HarmonySqlite] not opened, call open(dbName) first');
    }
    return this.store;
  }
}
