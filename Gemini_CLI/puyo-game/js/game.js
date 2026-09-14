/**
 * js/game.js
 * ゲームループ、対戦マネージャー、シーン遷移、おじゃまぷよ送受・相殺システム、入力管理
 */

class GameManager {
  constructor() {
    // 盤面インスタンス
    this.playerBoard = new Board(false);
    this.cpuBoard = new Board(true);

    // AI
    this.cpuAI = new CpuAI(this.cpuBoard);

    // パーティクルシステム
    this.playerParticles = new ParticleSystem();
    this.cpuParticles = new ParticleSystem();

    // Canvas & Context
    this.playerCanvas = document.getElementById('player-canvas');
    this.playerCtx = this.playerCanvas.getContext('2d');

    this.cpuCanvas = document.getElementById('cpu-canvas');
    this.cpuCtx = this.cpuCanvas.getContext('2d');

    this.playerNextCanvas = document.getElementById('player-next-canvas');
    this.playerNextCtx = this.playerNextCanvas.getContext('2d');

    this.cpuNextCanvas = document.getElementById('cpu-next-canvas');
    this.cpuNextCtx = this.cpuNextCanvas.getContext('2d');

    // ゲーム状態
    this.gameState = 'TITLE'; // TITLE, READY, PLAYING, PAUSED, STAGE_CLEAR, GAMEOVER, ALL_CLEAR
    this.currentStage = 1;
    this.maxStage = 5;
    this.gameTime = 0; // 秒数
    this.gameTicks = 0;

    // 入力状態
    this.keys = {};
    this.keyRepeatTimers = {};
    this.dasDelay = 10; // キーリピート開始までのディレイ
    this.dasInterval = 3; // リピート間隔

    // アニメーションフレームID
    this.rafId = null;

    // コールバック登録
    this.setupBoardCallbacks();
    this.setupInputListeners();
  }

  /** ボードからのイベントコールバック設定 */
  setupBoardCallbacks() {
    // プレイヤー側
    this.playerBoard.onLandCallback = () => {
      soundEngine.playLand();
    };

    this.playerBoard.onChainCallback = (chainCount, matches) => {
      soundEngine.playChain(chainCount);
      visualRenderer.triggerShake(Math.min(chainCount * 3 + 2, 20));

      // パーティクル発生
      for (const p of matches.puyos) {
        const cx = p.c * visualRenderer.cellSize + visualRenderer.cellSize / 2;
        const cy = (p.r - 1) * visualRenderer.cellSize + visualRenderer.cellSize / 2;
        this.playerParticles.explode(cx, cy, this.playerBoard.grid[p.r][p.c]);
      }
      for (const o of matches.ojamas) {
        const cx = o.c * visualRenderer.cellSize + visualRenderer.cellSize / 2;
        const cy = (o.r - 1) * visualRenderer.cellSize + visualRenderer.cellSize / 2;
        this.playerParticles.explode(cx, cy, 9, 14);
      }

      // カットイン演出
      this.showCutin('player-cutin', chainCount);
    };

    this.playerBoard.onOjamaDropCallback = () => {
      soundEngine.playOjamaDrop();
      visualRenderer.triggerShake(10);
    };

    // CPU側
    this.cpuBoard.onLandCallback = () => {
      // CPU設置音
    };

    this.cpuBoard.onChainCallback = (chainCount, matches) => {
      soundEngine.playChain(chainCount);
      // パーティクル
      for (const p of matches.puyos) {
        const cx = p.c * visualRenderer.cellSize + visualRenderer.cellSize / 2;
        const cy = (p.r - 1) * visualRenderer.cellSize + visualRenderer.cellSize / 2;
        this.cpuParticles.explode(cx, cy, this.cpuBoard.grid[p.r][p.c]);
      }
      for (const o of matches.ojamas) {
        const cx = o.c * visualRenderer.cellSize + visualRenderer.cellSize / 2;
        const cy = (o.r - 1) * visualRenderer.cellSize + visualRenderer.cellSize / 2;
        this.cpuParticles.explode(cx, cy, 9, 14);
      }

      this.showCutin('cpu-cutin', chainCount);
    };

    this.cpuBoard.onOjamaDropCallback = () => {
      soundEngine.playOjamaDrop();
    };
  }

  /**
   * カットイン演出の表示
   */
  showCutin(layerId, chainCount) {
    const layer = document.getElementById(layerId);
    if (!layer) return;

    let text = `${chainCount} CHAIN!`;
    if (chainCount >= 5) text = `FEVER ${chainCount}!! 🔥`;
    else if (chainCount >= 4) text = `AMAZING ${chainCount}!! ✨`;
    else if (chainCount >= 3) text = `GREAT ${chainCount}! ⭐`;

    const el = document.createElement('div');
    el.className = 'cutin-text';
    el.textContent = text;
    layer.appendChild(el);

    setTimeout(() => {
      if (el.parentNode) el.parentNode.removeChild(el);
    }, 950);
  }

  /** 入力キーリスナー設定 */
  setupInputListeners() {
    window.addEventListener('keydown', (e) => {
      soundEngine.resume();

      if (e.key === 'p' || e.key === 'P' || e.key === 'Escape') {
        this.togglePause();
        return;
      }

      if (this.gameState !== 'PLAYING') return;

      // 矢印キーでの画面スクロール防止
      if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', ' '].includes(e.key)) {
        e.preventDefault();
      }

      if (!this.keys[e.code]) {
        this.keys[e.code] = true;
        this.handleKeyDown(e.code);
      }
    });

    window.addEventListener('keyup', (e) => {
      this.keys[e.code] = false;
      delete this.keyRepeatTimers[e.code];
    });

    // モバイル・タッチボタン操作バインド
    const bindTouchBtn = (id, onDown, onHold) => {
      const btn = document.getElementById(id);
      if (!btn) return;

      const trigger = (e) => {
        e.preventDefault();
        soundEngine.resume();
        if (this.gameState !== 'PLAYING') return;
        onDown();
      };

      btn.addEventListener('touchstart', trigger, { passive: false });
      btn.addEventListener('mousedown', trigger);
    };

    bindTouchBtn('btn-left', () => {
      if (this.playerBoard.move(-1)) soundEngine.playMove();
    });
    bindTouchBtn('btn-right', () => {
      if (this.playerBoard.move(1)) soundEngine.playMove();
    });
    bindTouchBtn('btn-down', () => {
      if (this.playerBoard.dropStep()) soundEngine.playMove();
    });
    bindTouchBtn('btn-hard-drop', () => {
      this.playerBoard.hardDrop();
    });
    bindTouchBtn('btn-rot-l', () => {
      if (this.playerBoard.rotate(-1)) soundEngine.playRotate();
    });
    bindTouchBtn('btn-rot-r', () => {
      if (this.playerBoard.rotate(1)) soundEngine.playRotate();
    });
  }

  /** キー押下時アクション */
  handleKeyDown(code) {
    if (this.playerBoard.state !== 'CONTROLLING') return;

    switch (code) {
      case 'ArrowLeft':
      case 'KeyA':
        if (this.playerBoard.move(-1)) soundEngine.playMove();
        this.keyRepeatTimers[code] = { delay: this.dasDelay };
        break;

      case 'ArrowRight':
      case 'KeyD':
        if (this.playerBoard.move(1)) soundEngine.playMove();
        this.keyRepeatTimers[code] = { delay: this.dasDelay };
        break;

      case 'ArrowDown':
      case 'KeyS':
        if (this.playerBoard.dropStep()) soundEngine.playMove();
        this.keyRepeatTimers[code] = { delay: 4 };
        break;

      case 'ArrowUp':
      case 'Space':
        this.playerBoard.hardDrop();
        break;

      case 'KeyZ':
      case 'KeyJ':
        if (this.playerBoard.rotate(-1)) soundEngine.playRotate();
        break;

      case 'KeyX':
      case 'KeyK':
      case 'KeyW':
        if (this.playerBoard.rotate(1)) soundEngine.playRotate();
        break;
    }
  }

  /** キーリピート（長押し）の更新 */
  updateKeyRepeats() {
    if (this.playerBoard.state !== 'CONTROLLING') return;

    for (const code in this.keyRepeatTimers) {
      if (!this.keys[code]) continue;
      const timer = this.keyRepeatTimers[code];
      timer.delay--;

      if (timer.delay <= 0) {
        if (code === 'ArrowLeft' || code === 'KeyA') {
          if (this.playerBoard.move(-1)) soundEngine.playMove();
          timer.delay = this.dasInterval;
        } else if (code === 'ArrowRight' || code === 'KeyD') {
          if (this.playerBoard.move(1)) soundEngine.playMove();
          timer.delay = this.dasInterval;
        } else if (code === 'ArrowDown' || code === 'KeyS') {
          if (this.playerBoard.dropStep()) soundEngine.playMove();
          timer.delay = 2;
        }
      }
    }
  }

  /** ゲーム開始 / ステージ開始 */
  startStage(stage = 1) {
    this.currentStage = stage;
    this.gameState = 'READY';

    // ボード初期化
    const colors = CPU_PROFILES[stage].colorsCount || 4;
    this.playerBoard.reset(colors);
    this.cpuBoard.reset(colors);

    // AI設定
    this.cpuAI.setStage(stage);

    // UI更新
    this.updateStageInfoUI();
    this.setAnnouncer(`STAGE ${stage} READY...`);

    // BGM開始
    soundEngine.startBGM(stage);

    // 1.5秒後にGO!
    let readyTicks = 60;
    const readyInterval = setInterval(() => {
      readyTicks--;
      if (readyTicks === 30) {
        this.setAnnouncer('GO!! 🔥');
      } else if (readyTicks <= 0) {
        clearInterval(readyInterval);
        this.gameState = 'PLAYING';
        this.setAnnouncer('FIGHT!');
        setTimeout(() => this.setAnnouncer(''), 1000);
      }
    }, 25);

    this.startLoop();
  }

  /** ステージUIの更新 */
  updateStageInfoUI() {
    const profile = CPU_PROFILES[this.currentStage];
    document.getElementById('current-stage-num').textContent = `${this.currentStage} / ${this.maxStage}`;
    document.getElementById('current-stage-title').textContent = profile.name;

    const cpuName = document.getElementById('cpu-name');
    const cpuLevel = document.getElementById('cpu-level');
    const cpuAvatar = document.getElementById('cpu-avatar');

    if (cpuName) cpuName.textContent = profile.name;
    if (cpuLevel) cpuLevel.textContent = profile.title;
    if (cpuAvatar) cpuAvatar.textContent = profile.avatar;
  }

  /** アナウンサー文字更新 */
  setAnnouncer(text) {
    const el = document.getElementById('match-announcer');
    if (el) el.textContent = text;
  }

  /** 一時停止切り替え */
  togglePause() {
    if (this.gameState === 'PLAYING') {
      this.gameState = 'PAUSED';
      document.getElementById('pause-modal').classList.add('active');
    } else if (this.gameState === 'PAUSED') {
      this.gameState = 'PLAYING';
      document.getElementById('pause-modal').classList.remove('active');
    }
  }

  /** メインゲームループ */
  startLoop() {
    if (this.rafId) cancelAnimationFrame(this.rafId);

    const loop = () => {
      this.update();
      this.render();
      this.rafId = requestAnimationFrame(loop);
    };
    this.rafId = requestAnimationFrame(loop);
  }

  /** ゲームロジック更新 */
  update() {
    this.gameTicks++;
    visualRenderer.updateTime(this.gameTicks);

    if (this.gameState !== 'PLAYING') return;

    // タイマー計算
    if (this.gameTicks % 60 === 0) {
      this.gameTime++;
      const min = String(Math.floor(this.gameTime / 60)).padStart(2, '0');
      const sec = String(this.gameTime % 60).padStart(2, '0');
      document.getElementById('game-timer').textContent = `${min}:${sec}`;
    }

    // キーリピート入力処理
    this.updateKeyRepeats();

    // プレイヤー側のボード更新
    const pResult = this.playerBoard.update(4);
    if (pResult.chainOccurred && pResult.attackPower > 0) {
      this.handleAttack(this.playerBoard, this.cpuBoard, pResult.attackPower);
    }

    // CPU AIの思考更新
    this.cpuAI.update(this.playerBoard.ojamaStock);

    // CPU側のボード更新
    const cResult = this.cpuBoard.update(4);
    if (cResult.chainOccurred && cResult.attackPower > 0) {
      this.handleAttack(this.cpuBoard, this.playerBoard, cResult.attackPower);
    }

    // 危険アラート音とヴィネット点滅
    const pDanger = this.playerBoard.isDanger();
    const cDanger = this.cpuBoard.isDanger();
    document.getElementById('player-danger').classList.toggle('active', pDanger);
    document.getElementById('cpu-danger').classList.toggle('active', cDanger);

    if (pDanger && this.gameTicks % 75 === 0) {
      soundEngine.playDangerAlert();
    }

    // 勝敗判定
    if (pResult.gameOver) {
      this.handleGameOver();
    } else if (cResult.gameOver) {
      this.handleStageClear();
    }

    // パーティクル更新
    this.playerParticles.update();
    this.cpuParticles.update();
  }

  /**
   * おじゃまぷよ送受＆「相殺（そうさい）」システム
   * @param {Board} attacker 攻撃者
   * @param {Board} defender 防御者
   * @param {number} attackPower 発生攻撃力（おじゃま数）
   */
  handleAttack(attacker, defender, attackPower) {
    let power = attackPower;

    // 1. 自身の予告おじゃまぷよストックとの相殺
    if (attacker.ojamaStock > 0) {
      if (power >= attacker.ojamaStock) {
        power -= attacker.ojamaStock;
        attacker.ojamaStock = 0;
        soundEngine.playSousai();
      } else {
        attacker.ojamaStock -= power;
        power = 0;
        soundEngine.playSousai();
      }
    }

    // 2. 残った攻撃力で相手へ攻撃
    if (power > 0) {
      defender.ojamaStock += power;
    }

    // トレイ表示の即時反映
    visualRenderer.updateOjamaTray('player-ojama-tray', this.playerBoard.ojamaStock);
    visualRenderer.updateOjamaTray('cpu-ojama-tray', this.cpuBoard.ojamaStock);
  }

  /** ステージクリア処理 */
  handleStageClear() {
    this.gameState = 'STAGE_CLEAR';
    soundEngine.stopBGM();
    soundEngine.playVictory();

    const isLastStage = (this.currentStage >= this.maxStage);

    setTimeout(() => {
      const modal = document.getElementById('result-modal');
      const title = document.getElementById('result-title');
      const badge = document.getElementById('result-badge');
      const btnNext = document.getElementById('btn-next-stage');
      const btnRetry = document.getElementById('btn-retry');

      document.getElementById('res-stage').textContent = `Stage ${this.currentStage} / ${this.maxStage}`;
      document.getElementById('res-score').textContent = this.playerBoard.score.toLocaleString();
      document.getElementById('res-maxchain').textContent = `${this.playerBoard.maxChain} Chains`;

      if (isLastStage) {
        title.textContent = '🎉 ALL CLEAR!! 🎉';
        badge.textContent = 'YOU ARE THE PUYO PUYO MASTER!';
        btnNext.style.display = 'none';
        btnRetry.style.display = 'block';
      } else {
        title.textContent = 'STAGE CLEAR!';
        badge.textContent = `DEFEATED ${CPU_PROFILES[this.currentStage].name}!`;
        btnNext.style.display = 'block';
        btnRetry.style.display = 'none';
      }

      modal.classList.add('active');
    }, 1200);
  }

  /** ゲームオーバー処理 */
  handleGameOver() {
    this.gameState = 'GAMEOVER';
    soundEngine.stopBGM();
    soundEngine.playDefeat();

    setTimeout(() => {
      const modal = document.getElementById('result-modal');
      const title = document.getElementById('result-title');
      const badge = document.getElementById('result-badge');
      const btnNext = document.getElementById('btn-next-stage');
      const btnRetry = document.getElementById('btn-retry');

      title.textContent = 'GAME OVER';
      badge.textContent = 'YOU LOSE...';
      btnNext.style.display = 'none';
      btnRetry.style.display = 'block';

      document.getElementById('res-stage').textContent = `Stage ${this.currentStage}`;
      document.getElementById('res-score').textContent = this.playerBoard.score.toLocaleString();
      document.getElementById('res-maxchain').textContent = `${this.playerBoard.maxChain} Chains`;

      modal.classList.add('active');
    }, 1200);
  }

  /** レンダリング更新 */
  render() {
    // 1. メイン盤面描画
    visualRenderer.renderBoard(this.playerCtx, this.playerBoard, this.playerParticles);
    visualRenderer.renderBoard(this.cpuCtx, this.cpuBoard, this.cpuParticles);

    // 2. ネクストぷよ描画
    visualRenderer.renderNext(this.playerNextCtx, this.playerBoard.nextQueue);
    visualRenderer.renderNext(this.cpuNextCtx, this.cpuBoard.nextQueue);

    // 3. おじゃまトレイDOM更新
    visualRenderer.updateOjamaTray('player-ojama-tray', this.playerBoard.ojamaStock);
    visualRenderer.updateOjamaTray('cpu-ojama-tray', this.cpuBoard.ojamaStock);

    // 4. スコア表示更新
    document.getElementById('player-score').textContent = this.playerBoard.score.toLocaleString();
    document.getElementById('player-max-chain').textContent = this.playerBoard.maxChain;
    document.getElementById('cpu-score').textContent = this.cpuBoard.score.toLocaleString();
    document.getElementById('cpu-max-chain').textContent = this.cpuBoard.maxChain;
  }
}
