/**
 * js/ai.js
 * CPU配置思考ルーチン（ステージ1〜5の思考深さ・連鎖狙い・ミス率・操作エミュレーション）
 */

const CPU_PROFILES = {
  1: {
    name: 'すらいむ君',
    title: 'ぷよビギナー',
    avatar: '🟢',
    dropInterval: 48,
    thinkDelay: 35, // 約600ms
    missRate: 0.35,
    colorsCount: 4,
    strategy: 'random_clustering'
  },
  2: {
    name: 'アミ',
    title: '見習い魔導士',
    avatar: '🧙‍♀️',
    dropInterval: 38,
    thinkDelay: 24, // 約400ms
    missRate: 0.18,
    colorsCount: 4,
    strategy: 'pair_grouping'
  },
  3: {
    name: 'レオン',
    title: '熱血剣士',
    avatar: '⚔️',
    dropInterval: 28,
    thinkDelay: 16, // 約260ms
    missRate: 0.08,
    colorsCount: 4,
    strategy: 'chain_hunter'
  },
  4: {
    name: 'シャドウ',
    title: '天才怪盗',
    avatar: '🎭',
    dropInterval: 20,
    thinkDelay: 8,  // 約130ms
    missRate: 0.0,
    colorsCount: 4,
    strategy: 'tactical_sim'
  },
  5: {
    name: 'サタンヴォイド',
    title: '暗黒魔王 [BOSS]',
    avatar: '👿',
    dropInterval: 12,
    thinkDelay: 4,   // 約65ms
    missRate: 0.0,
    colorsCount: 4,
    strategy: 'master_ai'
  }
};

class CpuAI {
  constructor(board) {
    this.board = board;
    this.stage = 1;
    this.profile = CPU_PROFILES[1];

    this.plan = null; // { targetX, targetRot }
    this.thinkingTimer = 0;
    this.actionTimer = 0;
  }

  /** ステージ難易度セット */
  setStage(stage) {
    this.stage = Math.max(1, Math.min(stage, 5));
    this.profile = CPU_PROFILES[this.stage];
    this.board.dropInterval = this.profile.dropInterval;
    this.plan = null;
    this.thinkingTimer = 0;
  }

  /**
   * 思考と入力エミュレーションの毎フレーム更新
   * @param {number} playerOjamaStock プレイヤー側の予告おじゃまぷよ状況
   */
  update(playerOjamaStock = 0) {
    if (this.board.state !== 'CONTROLLING' || !this.board.activePiece) {
      this.plan = null;
      return;
    }

    // 思考中フェーズ
    if (!this.plan) {
      this.thinkingTimer++;
      if (this.thinkingTimer >= this.profile.thinkDelay) {
        this.plan = this.calculateBestMove(playerOjamaStock);
        this.thinkingTimer = 0;
        this.actionTimer = 0;
      }
      return;
    }

    // 操作実行フェーズ (スムーズなキー操作をエミュレート)
    this.actionTimer++;
    const actionInterval = Math.max(2, Math.floor(this.profile.dropInterval / 8));
    if (this.actionTimer < actionInterval) return;
    this.actionTimer = 0;

    const piece = this.board.activePiece;

    // 1. 回転合わせ
    if (piece.rot !== this.plan.targetRot) {
      // 最適な回転方向（時計回りか反時計回り）を選択
      const diff = (this.plan.targetRot - piece.rot + 4) % 4;
      if (diff === 1 || diff === 2) {
        this.board.rotate(1);
      } else {
        this.board.rotate(-1);
      }
      return;
    }

    // 2. 左右移動合わせ
    if (piece.x < this.plan.targetX) {
      this.board.move(1);
      return;
    } else if (piece.x > this.plan.targetX) {
      this.board.move(-1);
      return;
    }

    // 3. 配置位置に到達したら高速落下・ハードドロップ
    if (this.stage >= 3) {
      this.board.hardDrop();
    } else {
      this.board.dropStep();
    }
  }

  /**
   * 最適手の探索と評価
   */
  calculateBestMove(playerOjamaStock) {
    const piece = this.board.activePiece;
    const candidates = [];

    // ミス判定 (Stage 1〜3でミス率に応じてランダム配置)
    if (Math.random() < this.profile.missRate) {
      const rndX = Math.floor(Math.random() * COLS);
      const rndRot = Math.floor(Math.random() * 4);
      return { targetX: rndX, targetRot: rndRot };
    }

    // 全列 (0〜5) × 全回転 (0〜3) の候補を評価
    for (let rot = 0; rot < 4; rot++) {
      for (let x = 0; x < COLS; x++) {
        // この (x, rot) が着地可能かシミュレーション
        const simResult = this.simulatePlacement(x, rot, piece.colorA, piece.colorB);
        if (simResult.valid) {
          const score = this.evaluateBoard(simResult, playerOjamaStock);
          candidates.push({
            targetX: x,
            targetRot: rot,
            score: score
          });
        }
      }
    }

    if (candidates.length === 0) {
      return { targetX: 2, targetRot: 0 };
    }

    // スコア降順ソート
    candidates.sort((a, b) => b.score - a.score);

    // Stage 1〜2は最善手以外の候補もたまに選ぶ
    if (this.stage === 1 && candidates.length > 2) {
      const pickIdx = Math.floor(Math.random() * Math.min(3, candidates.length));
      return candidates[pickIdx];
    }

    return candidates[0];
  }

  /**
   * 仮想配置シミュレーション
   */
  simulatePlacement(targetX, rot, colorA, colorB) {
    // 盤面のクローン作成
    const virtualGrid = this.board.grid.map(row => [...row]);

    // 子ぷよの相対オフセット
    let dx = 0, dy = -1;
    if (rot === 1) { dx = 1; dy = 0; }
    else if (rot === 2) { dx = 0; dy = 1; }
    else if (rot === 3) { dx = -1; dy = 0; }

    const xA = targetX;
    const xB = targetX + dx;

    // 盤面外チェック
    if (xA < 0 || xA >= COLS || xB < 0 || xB >= COLS) {
      return { valid: false };
    }

    // 各列の現在の最上空きマス（接地マス）を見つける
    const getLandingY = (col, startY = 12) => {
      for (let r = startY; r >= 0; r--) {
        if (virtualGrid[r][col] === 0) return r;
      }
      return -1;
    };

    let yA, yB;

    if (dx === 0) {
      // 縦並び配置
      const bottomCol = xA;
      const groundY = getLandingY(bottomCol);
      if (groundY < 1) return { valid: false }; // 溢れ

      if (dy === -1) {
        // Aが下、Bが上
        yA = groundY;
        yB = groundY - 1;
      } else {
        // Aが上、Bが下
        yB = groundY;
        yA = groundY - 1;
      }
    } else {
      // 横並び配置（ちぎり落下）
      yA = getLandingY(xA);
      yB = getLandingY(xB);
    }

    if (yA < 1 || yB < 1) return { valid: false };

    // 仮想盤面に配置
    virtualGrid[yA][xA] = colorA;
    virtualGrid[yB][xB] = colorB;

    // 重力落下シミュレーション
    this.virtualApplyGravity(virtualGrid);

    // 連鎖シミュレーション
    const chainSim = this.virtualSimulateChains(virtualGrid);

    return {
      valid: true,
      grid: chainSim.finalGrid,
      chains: chainSim.chains,
      clearedPuyos: chainSim.totalCleared,
      initialPlacedGrid: virtualGrid
    };
  }

  /** 仮想重力落下 */
  virtualApplyGravity(grid) {
    for (let c = 0; c < COLS; c++) {
      let writeRow = ROWS - 1;
      for (let r = ROWS - 1; r >= 0; r--) {
        if (grid[r][c] !== 0) {
          if (writeRow !== r) {
            grid[writeRow][c] = grid[r][c];
            grid[r][c] = 0;
          }
          writeRow--;
        }
      }
    }
  }

  /**
   * 仮想連鎖シミュレーション
   */
  virtualSimulateChains(grid) {
    let chains = 0;
    let totalCleared = 0;
    const simGrid = grid.map(r => [...r]);

    while (true) {
      // 4連結以上を検出
      const visited = Array.from({ length: ROWS }, () => Array(COLS).fill(false));
      const toClear = [];

      for (let r = 1; r < ROWS; r++) {
        for (let c = 0; c < COLS; c++) {
          const type = simGrid[r][c];
          if (type >= 1 && type <= 5 && !visited[r][c]) {
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
                  if (!visited[n.r][n.c] && simGrid[n.r][n.c] === type) {
                    visited[n.r][n.c] = true;
                    queue.push(n);
                  }
                }
              }
            }

            if (group.length >= 4) {
              toClear.push(...group);
            }
          }
        }
      }

      if (toClear.length === 0) break;

      chains++;
      totalCleared += toClear.length;
      for (const cell of toClear) {
        simGrid[cell.r][cell.c] = 0;
      }
      this.virtualApplyGravity(simGrid);
    }

    return {
      chains,
      totalCleared,
      finalGrid: simGrid
    };
  }

  /**
   * 盤面総合評価関数
   */
  evaluateBoard(simResult, playerOjamaStock) {
    let score = 0;
    const grid = simResult.initialPlacedGrid;
    const finalGrid = simResult.grid;
    const chains = simResult.chains;

    // 1. 連鎖スコア
    if (this.stage === 1) {
      // Stage 1: とりあえず消えたら少し喜ぶ
      score += chains * 80;
    } else if (this.stage === 2 || this.stage === 3) {
      // Stage 2〜3: 2〜3連鎖を強く狙う
      if (chains >= 2) {
        score += chains * 600;
      } else if (chains === 1) {
        // 単発消しは少し減点（無駄に消さない）
        score -= 80;
      }
    } else {
      // Stage 4〜5: 相手におじゃま予告があれば即時相殺、なければ大連鎖重視
      if (this.board.ojamaStock > 0) {
        // 緊急相殺モード
        score += chains * 900;
      } else {
        // 大連鎖狙い（4連鎖以上で莫大な加点）
        if (chains >= 4) {
          score += chains * 2000;
        } else if (chains > 0 && chains < 3) {
          score -= 300; // 暴発ペナルティ
        }
      }
    }

    // 2. 同色隣接ポテンシャル (2連結・3連結) の評価
    const connectionScore = this.evaluateConnections(grid);
    score += connectionScore * (this.stage >= 3 ? 15 : 8);

    // 3. 3列目（窒息列）の安全性
    const colHeights = this.getColumnHeights(grid);
    const deadColHeight = colHeights[DEAD_COL];
    if (deadColHeight >= 10) {
      score -= 3000; // 非常に危険
    } else if (deadColHeight >= 8) {
      score -= 800;
    }

    // 4. 盤面の平坦度（凹凸ペナルティ）
    let roughness = 0;
    for (let c = 0; c < COLS - 1; c++) {
      roughness += Math.abs(colHeights[c] - colHeights[c + 1]);
    }
    score -= roughness * (this.stage >= 4 ? 20 : 10);

    // 5. 平均的な高さの管理
    const avgHeight = colHeights.reduce((a, b) => a + b, 0) / COLS;
    if (avgHeight > 9) {
      score -= (avgHeight - 9) * 200;
    }

    return score;
  }

  /** 同色隣接グループの評価 */
  evaluateConnections(grid) {
    let points = 0;
    const visited = Array.from({ length: ROWS }, () => Array(COLS).fill(false));

    for (let r = 1; r < ROWS; r++) {
      for (let c = 0; c < COLS; c++) {
        const type = grid[r][c];
        if (type >= 1 && type <= 5 && !visited[r][c]) {
          let count = 0;
          const queue = [{ r, c }];
          visited[r][c] = true;

          while (queue.length > 0) {
            const cur = queue.shift();
            count++;
            const neighbors = [
              { r: cur.r - 1, c: cur.c },
              { r: cur.r + 1, c: cur.c },
              { r: cur.r, c: cur.c - 1 },
              { r: cur.r, c: cur.c + 1 }
            ];
            for (const n of neighbors) {
              if (n.r >= 1 && n.r < ROWS && n.c >= 0 && n.c < COLS) {
                if (!visited[n.r][n.c] && grid[n.r][n.c] === type) {
                  visited[n.r][n.c] = true;
                  queue.push(n);
                }
              }
            }
          }

          if (count === 2) points += 25; // 2連結
          if (count === 3) points += 60; // 3連結（あと1個で連鎖）
        }
      }
    }
    return points;
  }

  /** 各列の高さ取得 */
  getColumnHeights(grid) {
    const heights = Array(COLS).fill(0);
    for (let c = 0; c < COLS; c++) {
      for (let r = 1; r < ROWS; r++) {
        if (grid[r][c] !== 0) {
          heights[c] = ROWS - r;
          break;
        }
      }
    }
    return heights;
  }
}
