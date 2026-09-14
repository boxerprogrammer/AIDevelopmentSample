/**
 * js/board.js
 * 盤面データ（横6×縦13）、操作ぷよ、壁蹴り、ちぎり落下、BFS連結探索、連鎖解決ロジック
 */

const COLS = 6;
const ROWS = 13; // 0行目は上部隠し段、1〜12行目が可視段
const DEAD_COL = 2; // 3列目（0-indexedで2）が窒息判定列

// 連鎖数ボーナステーブル
const CHAIN_BONUS = [
  0,   0,   8,  16,  32,  64,  96, 128, 160, 192,
  224, 256, 288, 320, 352, 384, 416, 448, 480, 512
];

// 連結数ボーナステーブル
const CONNECT_BONUS = {
  4: 0, 5: 2, 6: 3, 7: 4, 8: 5, 9: 6, 10: 7, 11: 10
};

// 色数ボーナステーブル
const COLOR_BONUS = [0, 0, 3, 6, 12, 24];

/**
 * 操作中の組ぷよ（2個1組）
 */
class ActivePiece {
  constructor(colorA, colorB, startX = 2, startY = 1) {
    this.x = startX; // 軸ぷよの列 (0〜5)
    this.y = startY; // 軸ぷよの行 (0〜12)
    this.colorA = colorA; // 軸ぷよの色
    this.colorB = colorB; // 子ぷよの色
    this.rot = 0; // 0: 上, 1: 右, 2: 下, 3: 左
  }

  /** 子ぷよの相対オフセット取得 */
  getPartnerOffset(rot = this.rot) {
    switch (rot) {
      case 0: return { dx: 0, dy: -1 }; // 上
      case 1: return { dx: 1, dy: 0 };  // 右
      case 2: return { dx: 0, dy: 1 };  // 下
      case 3: return { dx: -1, dy: 0 }; // 左
      default: return { dx: 0, dy: -1 };
    }
  }

  /** ぷよ2つの盤面絶対セル座標配列を取得 */
  getCells(x = this.x, y = this.y, rot = this.rot) {
    const offset = this.getPartnerOffset(rot);
    return [
      { x: x, y: y, type: this.colorA, isPivot: true },
      { x: x + offset.dx, y: y + offset.dy, type: this.colorB, isPivot: false }
    ];
  }
}

/**
 * 盤面管理クラス
 */
class Board {
  constructor(isCpu = false) {
    this.isCpu = isCpu;
    // 13行 × 6列 (0: 空, 1〜5: 色ぷよ, 9: おじゃまぷよ)
    this.grid = Array.from({ length: ROWS }, () => Array(COLS).fill(0));

    this.activePiece = null;
    this.nextQueue = []; // [{ colorA, colorB }, ...]
    this.ojamaStock = 0; // 相手から送られてきた予告おじゃまぷよ数
    this.ojamaRemainderScore = 0; // 70で割り切れなかった余りスコア
    
    // スコア・統計
    this.score = 0;
    this.currentChain = 0;
    this.maxChain = 0;
    this.isAllClear = false; // 全消しフラグ
    this.allClearBonusPending = false; // 次回攻撃に30個上乗せ

    // 状態管理
    this.state = 'SPAWN'; // SPAWN, CONTROLLING, DROPPING, CLEARING, RESOLVING, OJAMA_DROP, GAMEOVER
    this.stateTimer = 0;
    this.clearingCells = []; // 現在消去アニメーション中のセル一覧
    this.clearingStep = 0;

    // 落下カウンター
    this.dropInterval = 45; // 通常落下速度（フレーム数）
    this.dropTimer = 0;

    // 連鎖コールバック (SE, パーティクル, カットイン用)
    this.onChainCallback = null;
    this.onLandCallback = null;
    this.onOjamaDropCallback = null;
  }

  /** リセット */
  reset(colorsCount = 4) {
    this.grid = Array.from({ length: ROWS }, () => Array(COLS).fill(0));
    this.activePiece = null;
    this.nextQueue = [];
    this.ojamaStock = 0;
    this.ojamaRemainderScore = 0;
    this.score = 0;
    this.currentChain = 0;
    this.maxChain = 0;
    this.isAllClear = false;
    this.allClearBonusPending = false;
    this.state = 'SPAWN';
    this.stateTimer = 0;
    this.clearingCells = [];
    this.dropTimer = 0;

    // キュー初期補充 (Next 3組分)
    for (let i = 0; i < 3; i++) {
      this.nextQueue.push(this.generatePair(colorsCount));
    }
  }

  /** ランダムな組ぷよ生成 */
  generatePair(colorsCount = 4) {
    const pickColor = () => Math.floor(Math.random() * colorsCount) + 1;
    return {
      colorA: pickColor(),
      colorB: pickColor()
    };
  }

  /** 新しい操作ぷよを取り出す */
  spawnPiece(colorsCount = 4) {
    if (this.nextQueue.length < 2) {
      this.nextQueue.push(this.generatePair(colorsCount));
    }
    const next = this.nextQueue.shift();
    this.nextQueue.push(this.generatePair(colorsCount));

    // 出現位置: 3列目(x=2), 1行目(y=1)
    this.activePiece = new ActivePiece(next.colorA, next.colorB, 2, 1);

    // 出現マスが既に埋まっている場合はゲームオーバー
    if (this.grid[1][2] !== 0) {
      this.state = 'GAMEOVER';
      return false;
    }

    this.state = 'CONTROLLING';
    this.dropTimer = 0;
    return true;
  }

  /** セルが空かどうかの判定 */
  isEmpty(x, y) {
    if (x < 0 || x >= COLS || y < 0 || y >= ROWS) return false;
    return this.grid[y][x] === 0;
  }

  /** 指定配置が有効かチェック */
  isValidPosition(x, y, rot) {
    if (!this.activePiece) return false;
    const cells = this.activePiece.getCells(x, y, rot);
    for (const c of cells) {
      if (c.x < 0 || c.x >= COLS || c.y < 0 || c.y >= ROWS) return false;
      if (this.grid[c.y][c.x] !== 0) return false;
    }
    return true;
  }

  /** 左右移動 */
  move(dir) {
    if (this.state !== 'CONTROLLING' || !this.activePiece) return false;
    const nextX = this.activePiece.x + dir;
    if (this.isValidPosition(nextX, this.activePiece.y, this.activePiece.rot)) {
      this.activePiece.x = nextX;
      return true;
    }
    return false;
  }

  /**
   * 回転 (壁蹴り・床蹴り・反転キック対応)
   * @param {number} dir 1: 時計回り(右), -1: 反時計回り(左)
   */
  rotate(dir) {
    if (this.state !== 'CONTROLLING' || !this.activePiece) return false;
    const newRot = (this.activePiece.rot + dir + 4) % 4;

    // 1. その場で回転可能か
    if (this.isValidPosition(this.activePiece.x, this.activePiece.y, newRot)) {
      this.activePiece.rot = newRot;
      return true;
    }

    // 2. 壁蹴り (左右に1マスずらして試行)
    const kickOffsets = [-1, 1, 0];
    for (const dx of kickOffsets) {
      if (this.isValidPosition(this.activePiece.x + dx, this.activePiece.y, newRot)) {
        this.activePiece.x += dx;
        this.activePiece.rot = newRot;
        return true;
      }
    }

    // 3. 上に1マスずらす (床蹴り・接地時)
    if (this.isValidPosition(this.activePiece.x, this.activePiece.y - 1, newRot)) {
      this.activePiece.y -= 1;
      this.activePiece.rot = newRot;
      return true;
    }

    // 4. 左右が狭い場合の180度ダブルキック反転
    const oppositeRot = (this.activePiece.rot + 2) % 4;
    if (this.isValidPosition(this.activePiece.x, this.activePiece.y - 1, oppositeRot)) {
      this.activePiece.y -= 1;
      this.activePiece.rot = oppositeRot;
      return true;
    }

    return false;
  }

  /** 下移動 (ソフトドロップ) */
  dropStep() {
    if (this.state !== 'CONTROLLING' || !this.activePiece) return false;
    if (this.isValidPosition(this.activePiece.x, this.activePiece.y + 1, this.activePiece.rot)) {
      this.activePiece.y += 1;
      return true;
    } else {
      // 接地・固定へ
      this.lockPiece();
      return false;
    }
  }

  /** ハードドロップ (即時設置) */
  hardDrop() {
    if (this.state !== 'CONTROLLING' || !this.activePiece) return;
    while (this.isValidPosition(this.activePiece.x, this.activePiece.y + 1, this.activePiece.rot)) {
      this.activePiece.y += 1;
      this.score += 1; // ドロップボーナス
    }
    this.lockPiece();
  }

  /** ゴーストぷよのY座標を算出 */
  getGhostY(piece) {
    let gy = piece.y;
    while (this.isValidPosition(piece.x, gy + 1, piece.rot)) {
      gy++;
    }
    return gy;
  }

  /**
   * ぷよ固定＆「ちぎり落下（Free Fall）」の適用
   */
  lockPiece() {
    if (!this.activePiece) return;

    const cells = this.activePiece.getCells();
    this.activePiece = null;

    if (this.onLandCallback) this.onLandCallback();

    // 盤面に配置
    for (const c of cells) {
      if (c.y >= 0 && c.y < ROWS && c.x >= 0 && c.x < COLS) {
        this.grid[c.y][c.x] = c.type;
      }
    }

    // ちぎり落下の適用（空中に浮いているぷよを自由落下）
    this.applyGravity();

    // 連鎖解決フェーズへ
    this.currentChain = 0;
    this.state = 'RESOLVING';
    this.stateTimer = 10;
  }

  /**
   * 重力落下（ちぎり落下）
   * 全列で浮いているぷよを下の空マスへ詰める
   * @returns {boolean} ぷよが移動したかどうか
   */
  applyGravity() {
    let moved = false;
    for (let c = 0; c < COLS; c++) {
      let writeRow = ROWS - 1;
      for (let r = ROWS - 1; r >= 0; r--) {
        if (this.grid[r][c] !== 0) {
          if (writeRow !== r) {
            this.grid[writeRow][c] = this.grid[r][c];
            this.grid[r][c] = 0;
            moved = true;
          }
          writeRow--;
        }
      }
    }
    return moved;
  }

  /**
   * 4個以上連結しているぷよの探索 (BFS)
   * および巻き込まれる隣接おじゃまぷよの探索
   * @returns {Object|null} 消去対象セルの情報
   */
  findMatches() {
    const visited = Array.from({ length: ROWS }, () => Array(COLS).fill(false));
    const matchedPuyos = [];
    const matchedColors = new Set();
    const groupSizes = [];

    // 1. 同色4連結以上の探索
    for (let r = 1; r < ROWS; r++) { // 0行目隠し段は消去判定対象外
      for (let c = 0; c < COLS; c++) {
        const type = this.grid[r][c];
        if (type >= 1 && type <= 5 && !visited[r][c]) {
          // BFS開始
          const group = [];
          const queue = [{ r, c }];
          visited[r][c] = true;

          while (queue.length > 0) {
            const cur = queue.shift();
            group.push(cur);

            const neighbors = [
              { r: cur.r - 1, c: cur.c },
              { r: cur.r + 1, c: cur.c },
              { r: cur.r, c: cur.c - 1 },
              { r: cur.r, c: cur.c + 1 }
            ];

            for (const n of neighbors) {
              if (n.r >= 1 && n.r < ROWS && n.c >= 0 && n.c < COLS) {
                if (!visited[n.r][n.c] && this.grid[n.r][n.c] === type) {
                  visited[n.r][n.c] = true;
                  queue.push(n);
                }
              }
            }
          }

          if (group.length >= 4) {
            matchedPuyos.push(...group);
            matchedColors.add(type);
            groupSizes.push(group.length);
          }
        }
      }
    }

    if (matchedPuyos.length === 0) return null;

    // 2. 消去される色ぷよに隣接する「おじゃまぷよ(9)」を巻き込み探索
    const matchedOjamas = [];
    const ojamaVisited = Array.from({ length: ROWS }, () => Array(COLS).fill(false));

    for (const cell of matchedPuyos) {
      const neighbors = [
        { r: cell.r - 1, c: cell.c },
        { r: cell.r + 1, c: cell.c },
        { r: cell.r, c: cell.c - 1 },
        { r: cell.r, c: cell.c + 1 }
      ];

      for (const n of neighbors) {
        if (n.r >= 1 && n.r < ROWS && n.c >= 0 && n.c < COLS) {
          if (this.grid[n.r][n.c] === 9 && !ojamaVisited[n.r][n.c]) {
            ojamaVisited[n.r][n.c] = true;
            matchedOjamas.push(n);
          }
        }
      }
    }

    return {
      puyos: matchedPuyos,
      ojamas: matchedOjamas,
      colorCount: matchedColors.size,
      groupSizes: groupSizes
    };
  }

  /**
   * 連鎖スコアとおじゃまぷよ生成数の計算
   */
  calculateAttackPower(matchData) {
    const chainIdx = Math.min(this.currentChain, CHAIN_BONUS.length - 1);
    const chainBonus = CHAIN_BONUS[chainIdx];

    // 連結ボーナス
    let connectBonus = 0;
    for (const size of matchData.groupSizes) {
      const clampedSize = Math.min(size, 11);
      connectBonus += CONNECT_BONUS[clampedSize] || 10;
    }

    // 色数ボーナス
    const colorBonus = COLOR_BONUS[matchData.colorCount] || 0;

    // 合計倍率 (0の場合は1)
    let totalMultiplier = chainBonus + connectBonus + colorBonus;
    if (totalMultiplier <= 0) totalMultiplier = 1;
    if (totalMultiplier > 999) totalMultiplier = 999;

    // 基礎点: 消去色ぷよ数 × 10
    const baseScore = matchData.puyos.length * 10;
    const stepScore = baseScore * totalMultiplier;
    this.score += stepScore;

    // おじゃまぷよ計算 (スコア / 70)
    const totalPoints = stepScore + this.ojamaRemainderScore;
    const generatedOjama = Math.floor(totalPoints / 70);
    this.ojamaRemainderScore = totalPoints % 70;

    return generatedOjama;
  }

  /**
   * 盤面の全消しチェック
   */
  checkAllClear() {
    for (let r = 1; r < ROWS; r++) {
      for (let c = 0; c < COLS; c++) {
        if (this.grid[r][c] !== 0) return false;
      }
    }
    return true;
  }

  /**
   * おじゃまぷよの降灰（最大30個＝5段分）
   * @param {number} count 降らせる個数
   * @returns {number} 実際に降った個数
   */
  dropGarbage(count) {
    if (count <= 0) return 0;
    const dropAmount = Math.min(count, 30); // 1回最大30個
    let remaining = dropAmount;

    // 各列均等に上から落とす (シャッフル列順)
    const colsOrder = [0, 1, 2, 3, 4, 5];
    // ランダムに少し崩す
    colsOrder.sort(() => Math.random() - 0.5);

    while (remaining > 0) {
      for (const col of colsOrder) {
        if (remaining <= 0) break;
        // 列の最上部（row 0 または空いている最上部）に9を設置
        if (this.grid[0][col] === 0) {
          this.grid[0][col] = 9;
          remaining--;
        }
      }
      // 落とす余地がなければ終了
      if (colsOrder.every(c => this.grid[0][c] !== 0)) break;
    }

    this.applyGravity();
    if (this.onOjamaDropCallback) this.onOjamaDropCallback();
    return dropAmount;
  }

  /**
   * メイン状態遷移更新（毎フレーム実行）
   * @param {number} colorsCount 使用色数
   * @returns {Object} { chainOccurred, attackPower, eventType }
   */
  update(colorsCount = 4) {
    const result = {
      chainOccurred: false,
      attackPower: 0,
      chainCount: 0,
      isAllClear: false,
      gameOver: false
    };

    if (this.state === 'GAMEOVER') {
      result.gameOver = true;
      return result;
    }

    // 1. 操作フェーズ
    if (this.state === 'CONTROLLING') {
      this.dropTimer++;
      if (this.dropTimer >= this.dropInterval) {
        this.dropTimer = 0;
        this.dropStep();
      }
      return result;
    }

    // 2. 連鎖解決フェーズ
    if (this.state === 'RESOLVING') {
      this.stateTimer--;
      if (this.stateTimer > 0) return result;

      // 重力落下をまず適用
      const moved = this.applyGravity();
      if (moved) {
        this.stateTimer = 10;
        return result;
      }

      // 4連結以上のグループ探索
      const matches = this.findMatches();
      if (matches) {
        // 連鎖成立！
        this.currentChain++;
        if (this.currentChain > this.maxChain) {
          this.maxChain = this.currentChain;
        }

        // 攻撃力計算
        let attack = this.calculateAttackPower(matches);

        // 前回全消しボーナスがあれば上乗せ
        if (this.allClearBonusPending) {
          attack += 30; // 岩ぷよ1個分
          this.allClearBonusPending = false;
        }

        result.chainOccurred = true;
        result.attackPower = attack;
        result.chainCount = this.currentChain;

        // 消去アニメーションの開始
        this.clearingCells = [...matches.puyos, ...matches.ojamas];
        this.state = 'CLEARING';
        this.stateTimer = 22; // 22フレーム間ブルブル発光

        if (this.onChainCallback) {
          this.onChainCallback(this.currentChain, matches);
        }
      } else {
        // 連鎖終了！
        if (this.currentChain > 0) {
          // 全消し判定
          if (this.checkAllClear()) {
            this.isAllClear = true;
            this.allClearBonusPending = true;
            result.isAllClear = true;
          }
        }
        this.currentChain = 0;

        // おじゃまぷよ落下フェーズへ移行
        this.state = 'OJAMA_DROP';
        this.stateTimer = 12;
      }
      return result;
    }

    // 3. 消滅演出フェーズ
    if (this.state === 'CLEARING') {
      this.stateTimer--;
      if (this.stateTimer <= 0) {
        // 盤面から該当セルを完全に消去
        for (const c of this.clearingCells) {
          this.grid[c.r][c.c] = 0;
        }
        this.clearingCells = [];

        // 落下へ戻る
        this.state = 'RESOLVING';
        this.stateTimer = 12;
      }
      return result;
    }

    // 4. おじゃまぷよ降灰フェーズ
    if (this.state === 'OJAMA_DROP') {
      this.stateTimer--;
      if (this.stateTimer <= 0) {
        if (this.ojamaStock > 0) {
          const dropped = this.dropGarbage(this.ojamaStock);
          this.ojamaStock -= dropped;
          // 降ったおじゃまで連鎖が起きるか再チェックへ
          this.state = 'RESOLVING';
          this.stateTimer = 12;
          return result;
        }

        // おじゃま落下完了 -> 窒息判定
        if (this.grid[1][DEAD_COL] !== 0) {
          this.state = 'GAMEOVER';
          result.gameOver = true;
          return result;
        }

        // 次のぷよ召喚へ
        this.state = 'SPAWN';
      }
      return result;
    }

    // 5. 新ぷよ召喚フェーズ
    if (this.state === 'SPAWN') {
      const ok = this.spawnPiece(colorsCount);
      if (!ok) {
        result.gameOver = true;
      }
      return result;
    }

    return result;
  }

  /** 指定セルが現在消滅アニメーション中か判定 */
  isClearingCell(r, c) {
    return this.clearingCells.some(cell => cell.r === r && cell.c === c);
  }

  /** 上下左右の同色接続情報を取得 */
  getConnections(r, c) {
    const type = this.grid[r][c];
    if (type <= 0 || type === 9) { // おじゃまぷよは連結しない
      return { up: false, down: false, left: false, right: false };
    }

    return {
      up: (r > 1 && this.grid[r - 1][c] === type),
      down: (r < ROWS - 1 && this.grid[r + 1][c] === type),
      left: (c > 0 && this.grid[r][c - 1] === type),
      right: (c < COLS - 1 && this.grid[r][c + 1] === type)
    };
  }

  /** 3列目危険フラグ (ピンチ警告) */
  isDanger() {
    // 3列目の高さが10段以上（上から3マス以内）に達しているか
    return this.grid[3][DEAD_COL] !== 0 || this.grid[2][DEAD_COL] !== 0;
  }
}
