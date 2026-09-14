/**
 * TETRIS JS - Pure Vanilla JavaScript & HTML5 Canvas
 */

// --- 定数定義 ---
const COLS = 10;
const ROWS = 20;
const BLOCK_SIZE = 30; // 30px x 10 = 300px, 30px x 20 = 600px

// 7種類のテトリミノ定義（標準カラー＆シェード）
const TETROMINOES = {
  I: {
    shape: [
      [0, 0, 0, 0],
      [1, 1, 1, 1],
      [0, 0, 0, 0],
      [0, 0, 0, 0]
    ],
    color: '#00f0f0',
    light: '#a5f3fc',
    dark: '#0891b2'
  },
  O: {
    shape: [
      [1, 1],
      [1, 1]
    ],
    color: '#f0f000',
    light: '#fef08a',
    dark: '#ca8a04'
  },
  T: {
    shape: [
      [0, 1, 0],
      [1, 1, 1],
      [0, 0, 0]
    ],
    color: '#a000f0',
    light: '#e9d5ff',
    dark: '#7e22ce'
  },
  S: {
    shape: [
      [0, 1, 1],
      [1, 1, 0],
      [0, 0, 0]
    ],
    color: '#00f000',
    light: '#bbf7d0',
    dark: '#16a34a'
  },
  Z: {
    shape: [
      [1, 1, 0],
      [0, 1, 1],
      [0, 0, 0]
    ],
    color: '#f00000',
    light: '#fecaca',
    dark: '#dc2626'
  },
  J: {
    shape: [
      [1, 0, 0],
      [1, 1, 1],
      [0, 0, 0]
    ],
    color: '#0000f0',
    light: '#bfdbfe',
    dark: '#1d4ed8'
  },
  L: {
    shape: [
      [0, 0, 1],
      [1, 1, 1],
      [0, 0, 0]
    ],
    color: '#f0a000',
    light: '#fed7aa',
    dark: '#ea580c'
  }
};

// スコア計算テーブル (消去ライン数: 基本点)
const SCORE_TABLE = {
  1: 100,
  2: 300,
  3: 500,
  4: 800
};

// 壁蹴り（Wall Kick）テストオフセット
const WALL_KICK_OFFSETS = [
  [0, 0],
  [-1, 0],
  [1, 0],
  [0, -1],
  [-1, -1],
  [1, -1],
  [-2, 0],
  [2, 0],
  [0, -2]
];

// --- Web Audio API サウンドシステム ---
class SoundController {
  constructor() {
    this.ctx = null;
    this.masterGain = null;
    this.muted = false;
  }

  init() {
    if (!this.ctx) {
      const AudioCtx = window.AudioContext || window.webkitAudioContext;
      if (AudioCtx) {
        this.ctx = new AudioCtx();
        this.masterGain = this.ctx.createGain();
        this.masterGain.gain.setValueAtTime(0.2, this.ctx.currentTime);
        this.masterGain.connect(this.ctx.destination);
      }
    }
    if (this.ctx && this.ctx.state === 'suspended') {
      this.ctx.resume();
    }
  }

  toggleMute() {
    this.init();
    this.muted = !this.muted;
    if (this.masterGain && this.ctx) {
      this.masterGain.gain.setValueAtTime(this.muted ? 0 : 0.2, this.ctx.currentTime);
    }
    return this.muted;
  }

  playMove() {
    if (this.muted || !this.ctx) return;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();
    const t = this.ctx.currentTime;

    osc.type = 'sine';
    osc.frequency.setValueAtTime(320, t);
    osc.frequency.exponentialRampToValueAtTime(200, t + 0.04);

    gain.gain.setValueAtTime(0.3, t);
    gain.gain.exponentialRampToValueAtTime(0.01, t + 0.04);

    osc.connect(gain);
    gain.connect(this.masterGain);

    osc.start(t);
    osc.stop(t + 0.04);
  }

  playRotate() {
    if (this.muted || !this.ctx) return;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();
    const t = this.ctx.currentTime;

    osc.type = 'triangle';
    osc.frequency.setValueAtTime(380, t);
    osc.frequency.exponentialRampToValueAtTime(620, t + 0.06);

    gain.gain.setValueAtTime(0.35, t);
    gain.gain.exponentialRampToValueAtTime(0.01, t + 0.06);

    osc.connect(gain);
    gain.connect(this.masterGain);

    osc.start(t);
    osc.stop(t + 0.06);
  }

  playSoftDrop() {
    if (this.muted || !this.ctx) return;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();
    const t = this.ctx.currentTime;

    osc.type = 'sine';
    osc.frequency.setValueAtTime(140, t);
    osc.frequency.exponentialRampToValueAtTime(80, t + 0.03);

    gain.gain.setValueAtTime(0.2, t);
    gain.gain.exponentialRampToValueAtTime(0.01, t + 0.03);

    osc.connect(gain);
    gain.connect(this.masterGain);

    osc.start(t);
    osc.stop(t + 0.03);
  }

  playHardDrop() {
    if (this.muted || !this.ctx) return;
    const t = this.ctx.currentTime;

    // 低音キック
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();
    osc.type = 'sine';
    osc.frequency.setValueAtTime(180, t);
    osc.frequency.exponentialRampToValueAtTime(40, t + 0.12);

    gain.gain.setValueAtTime(0.8, t);
    gain.gain.exponentialRampToValueAtTime(0.01, t + 0.12);

    osc.connect(gain);
    gain.connect(this.masterGain);

    osc.start(t);
    osc.stop(t + 0.12);
  }

  playHold() {
    if (this.muted || !this.ctx) return;
    const t = this.ctx.currentTime;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();

    osc.type = 'square';
    osc.frequency.setValueAtTime(440, t);
    osc.frequency.setValueAtTime(660, t + 0.04);

    gain.gain.setValueAtTime(0.25, t);
    gain.gain.exponentialRampToValueAtTime(0.01, t + 0.09);

    osc.connect(gain);
    gain.connect(this.masterGain);

    osc.start(t);
    osc.stop(t + 0.09);
  }

  playClear(lines) {
    if (this.muted || !this.ctx) return;
    const t = this.ctx.currentTime;

    if (lines === 4) {
      // TETRIS: 華やかなアルペジオ (C5, E5, G5, C6)
      const freqs = [523.25, 659.25, 783.99, 1046.5];
      freqs.forEach((freq, idx) => {
        const osc = this.ctx.createOscillator();
        const gain = this.ctx.createGain();
        const noteTime = t + idx * 0.07;

        osc.type = 'triangle';
        osc.frequency.setValueAtTime(freq, noteTime);

        gain.gain.setValueAtTime(0, noteTime);
        gain.gain.linearRampToValueAtTime(0.5, noteTime + 0.02);
        gain.gain.exponentialRampToValueAtTime(0.01, noteTime + 0.25);

        osc.connect(gain);
        gain.connect(this.masterGain);

        osc.start(noteTime);
        osc.stop(noteTime + 0.25);
      });
    } else {
      // 通常ライン消去 (1~3行)
      const baseFreq = 440;
      const count = Math.min(lines, 3);
      for (let i = 0; i < count; i++) {
        const osc = this.ctx.createOscillator();
        const gain = this.ctx.createGain();
        const noteTime = t + i * 0.06;

        osc.type = 'sine';
        osc.frequency.setValueAtTime(baseFreq * Math.pow(1.26, i + 1), noteTime);

        gain.gain.setValueAtTime(0.4, noteTime);
        gain.gain.exponentialRampToValueAtTime(0.01, noteTime + 0.18);

        osc.connect(gain);
        gain.connect(this.masterGain);

        osc.start(noteTime);
        osc.stop(noteTime + 0.18);
      }
    }
  }

  playLevelUp() {
    if (this.muted || !this.ctx) return;
    const t = this.ctx.currentTime;
    const freqs = [440, 554.37, 659.25, 880];
    freqs.forEach((freq, idx) => {
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();
      const noteTime = t + idx * 0.08;

      osc.type = 'sawtooth';
      osc.frequency.setValueAtTime(freq, noteTime);

      gain.gain.setValueAtTime(0.25, noteTime);
      gain.gain.exponentialRampToValueAtTime(0.01, noteTime + 0.2);

      osc.connect(gain);
      gain.connect(this.masterGain);

      osc.start(noteTime);
      osc.stop(noteTime + 0.2);
    });
  }

  playGameOver() {
    if (this.muted || !this.ctx) return;
    const t = this.ctx.currentTime;
    const freqs = [392.00, 311.13, 261.63, 196.00];
    freqs.forEach((freq, idx) => {
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();
      const noteTime = t + idx * 0.12;

      osc.type = 'sawtooth';
      osc.frequency.setValueAtTime(freq, noteTime);

      gain.gain.setValueAtTime(0.4, noteTime);
      gain.gain.exponentialRampToValueAtTime(0.01, noteTime + 0.35);

      osc.connect(gain);
      gain.connect(this.masterGain);

      osc.start(noteTime);
      osc.stop(noteTime + 0.35);
    });
  }
}

// --- 7-Bag ランダマイザー ---
class SevenBagRandomizer {
  constructor() {
    this.bag = [];
  }

  refill() {
    const pieces = ['I', 'O', 'T', 'S', 'Z', 'J', 'L'];
    for (let i = pieces.length - 1; i > 0; i--) {
      const j = Math.floor(Math.random() * (i + 1));
      [pieces[i], pieces[j]] = [pieces[j], pieces[i]];
    }
    this.bag.push(...pieces);
  }

  next() {
    if (this.bag.length <= 7) {
      this.refill();
    }
    return this.bag.shift();
  }

  reset() {
    this.bag = [];
    this.refill();
    this.refill();
  }
}

// --- パーティクルシステム（エフェクト） ---
class ParticleSystem {
  constructor() {
    this.particles = [];
  }

  addSparkles(x, y, color, count = 12) {
    for (let i = 0; i < count; i++) {
      const angle = Math.random() * Math.PI * 2;
      const speed = Math.random() * 4 + 1.5;
      this.particles.push({
        x: x,
        y: y,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed - 1.5,
        size: Math.random() * 4 + 2,
        color: color,
        alpha: 1.0,
        decay: Math.random() * 0.03 + 0.02
      });
    }
  }

  update() {
    for (let i = this.particles.length - 1; i >= 0; i--) {
      const p = this.particles[i];
      p.x += p.vx;
      p.y += p.vy;
      p.vy += 0.12; // 重力
      p.alpha -= p.decay;
      if (p.alpha <= 0) {
        this.particles.splice(i, 1);
      }
    }
  }

  draw(ctx) {
    ctx.save();
    for (const p of this.particles) {
      ctx.globalAlpha = Math.max(0, p.alpha);
      ctx.fillStyle = p.color;
      ctx.beginPath();
      ctx.arc(p.x, p.y, p.size, 0, Math.PI * 2);
      ctx.fill();
    }
    ctx.restore();
  }

  clear() {
    this.particles = [];
  }
}

// --- ゲームメインクラス ---
class TetrisGame {
  constructor() {
    // Canvas & Contexts
    this.canvas = document.getElementById('tetrisCanvas');
    this.ctx = this.canvas.getContext('2d');
    this.holdCanvas = document.getElementById('holdCanvas');
    this.holdCtx = this.holdCanvas.getContext('2d');
    this.nextCanvas = document.getElementById('nextCanvas');
    this.nextCtx = this.nextCanvas.getContext('2d');

    // UI Elements
    this.scoreDisplay = document.getElementById('scoreDisplay');
    this.highScoreDisplay = document.getElementById('highScoreDisplay');
    this.levelDisplay = document.getElementById('levelDisplay');
    this.linesDisplay = document.getElementById('linesDisplay');
    this.overlay = document.getElementById('boardOverlay');
    this.overlayTitle = document.getElementById('overlayTitle');
    this.overlayMessage = document.getElementById('overlayMessage');
    this.overlayActionBtn = document.getElementById('overlayActionBtn');
    this.soundToggleBtn = document.getElementById('soundToggleBtn');
    this.soundIcon = document.getElementById('soundIcon');
    this.pauseBtn = document.getElementById('pauseBtn');
    this.pauseIcon = document.getElementById('pauseIcon');
    this.restartBtn = document.getElementById('restartBtn');
    this.banner = document.getElementById('bannerNotification');

    // コントローラー
    this.sound = new SoundController();
    this.randomizer = new SevenBagRandomizer();
    this.particles = new ParticleSystem();

    // ゲーム状態
    this.grid = this.createGrid();
    this.score = 0;
    this.highScore = parseInt(localStorage.getItem('tetris_high_score') || '0', 10);
    this.lines = 0;
    this.level = 1;
    this.currentPiece = null;
    this.nextQueue = [];
    this.holdPieceType = null;
    this.canHold = true;
    this.isGameOver = false;
    this.isPaused = false;
    this.isPlaying = false;

    // タイマー・アニメーション
    this.lastTime = 0;
    this.dropCounter = 0;
    this.dropInterval = 1000;
    this.lockDelayCounter = 0;
    this.lockDelayLimit = 500; // 接地後0.5秒の猶予
    this.isLocking = false;
    this.clearingLines = [];
    this.clearAnimationTimer = 0;

    // キー入力 DAS/ARR 制御
    this.keys = {};
    this.dasTimers = {};
    this.DAS_DELAY = 140; // 最初の長押し判定時間 (ms)
    this.ARR_RATE = 35;   // リピート間隔 (ms)

    this.initUI();
    this.bindEvents();
    this.updateStatsUI();
  }

  createGrid() {
    return Array.from({ length: ROWS }, () => Array(COLS).fill(0));
  }

  initUI() {
    this.highScoreDisplay.textContent = this.highScore.toLocaleString();
    this.showOverlay('TETRIS', 'スペースキーまたはボタンを押してスタート', 'GAME START');
  }

  showOverlay(title, message, btnText) {
    this.overlayTitle.textContent = title;
    this.overlayMessage.textContent = message;
    this.overlayActionBtn.textContent = btnText;
    this.overlay.classList.remove('hidden');
  }

  hideOverlay() {
    this.overlay.classList.add('hidden');
  }

  showBanner(text, type = 'tetris') {
    this.banner.textContent = text;
    this.banner.className = `banner-notification show ${type}`;
    setTimeout(() => {
      this.banner.classList.remove('show');
    }, 1200);
  }

  // --- ゲーム開始 & リセット ---
  start() {
    this.sound.init();
    this.grid = this.createGrid();
    this.score = 0;
    this.lines = 0;
    this.level = 1;
    this.dropInterval = this.getSpeedForLevel(this.level);
    this.isGameOver = false;
    this.isPaused = false;
    this.isPlaying = true;
    this.holdPieceType = null;
    this.canHold = true;
    this.clearingLines = [];
    this.particles.clear();

    this.randomizer.reset();
    this.nextQueue = [
      this.randomizer.next(),
      this.randomizer.next(),
      this.randomizer.next(),
      this.randomizer.next()
    ];

    this.spawnPiece();
    this.updateStatsUI();
    this.hideOverlay();

    this.lastTime = performance.now();
    requestAnimationFrame(this.gameLoop.bind(this));
  }

  restart() {
    this.start();
  }

  pause() {
    if (!this.isPlaying || this.isGameOver) return;
    this.isPaused = !this.isPaused;
    if (this.isPaused) {
      this.pauseIcon.textContent = '▶️';
      this.showOverlay('PAUSED', '一時停止中 - [P] またはボタンで再開', 'RESUME');
    } else {
      this.pauseIcon.textContent = '⏸️';
      this.hideOverlay();
      this.lastTime = performance.now();
      requestAnimationFrame(this.gameLoop.bind(this));
    }
  }

  getSpeedForLevel(level) {
    // レベルが上がるにつれて落下速度が速くなる (ms)
    return Math.max(100, Math.floor(1000 * Math.pow(0.85, level - 1)));
  }

  // --- ミノ出現 ---
  spawnPiece() {
    const type = this.nextQueue.shift();
    this.nextQueue.push(this.randomizer.next());

    const shape = TETROMINOES[type].shape.map(row => [...row]);
    this.currentPiece = {
      type: type,
      shape: shape,
      x: Math.floor((COLS - shape[0].length) / 2),
      y: 0
    };

    this.canHold = true;
    this.isLocking = false;
    this.lockDelayCounter = 0;

    // 出現時にすでに衝突している場合はゲームオーバー
    if (this.checkCollision(this.currentPiece.shape, this.currentPiece.x, this.currentPiece.y)) {
      this.gameOver();
    }
  }

  // --- ホールド機能 ---
  hold() {
    if (!this.canHold || this.isGameOver || this.isPaused || !this.isPlaying) return;

    this.sound.playHold();
    const currentType = this.currentPiece.type;

    if (this.holdPieceType === null) {
      this.holdPieceType = currentType;
      this.spawnPiece();
    } else {
      const nextType = this.holdPieceType;
      this.holdPieceType = currentType;
      const shape = TETROMINOES[nextType].shape.map(row => [...row]);
      this.currentPiece = {
        type: nextType,
        shape: shape,
        x: Math.floor((COLS - shape[0].length) / 2),
        y: 0
      };
    }

    this.canHold = false;
    this.isLocking = false;
    this.lockDelayCounter = 0;
  }

  // --- 衝突判定 ---
  checkCollision(shape, posX, posY) {
    for (let r = 0; r < shape.length; r++) {
      for (let c = 0; c < shape[r].length; c++) {
        if (shape[r][c] !== 0) {
          const newX = posX + c;
          const newY = posY + r;

          // 左右の壁
          if (newX < 0 || newX >= COLS) return true;
          // 床
          if (newY >= ROWS) return true;
          // 既存のブロックとの衝突
          if (newY >= 0 && this.grid[newY][newX] !== 0) return true;
        }
      }
    }
    return false;
  }

  // --- ミノ移動 ---
  move(dir) {
    if (!this.isPlaying || this.isPaused || this.isGameOver || !this.currentPiece) return false;

    if (!this.checkCollision(this.currentPiece.shape, this.currentPiece.x + dir, this.currentPiece.y)) {
      this.currentPiece.x += dir;
      this.sound.playMove();

      // 接地中に横移動した場合はロック猶予を少し延長
      if (this.isLocking) {
        this.lockDelayCounter = 0;
      }
      return true;
    }
    return false;
  }

  // --- ミノ回転（壁蹴り付き） ---
  rotate(dir = 1) {
    if (!this.isPlaying || this.isPaused || this.isGameOver || !this.currentPiece) return;

    const n = this.currentPiece.shape.length;
    const rotated = Array.from({ length: n }, () => Array(n).fill(0));

    // 行列回転 (dir === 1: 時計回り, dir === -1: 反時計回り)
    for (let r = 0; r < n; r++) {
      for (let c = 0; c < n; c++) {
        if (dir === 1) {
          rotated[c][n - 1 - r] = this.currentPiece.shape[r][c];
        } else {
          rotated[n - 1 - c][r] = this.currentPiece.shape[r][c];
        }
      }
    }

    // 壁蹴り（Wall Kick）テスト
    for (const [ox, oy] of WALL_KICK_OFFSETS) {
      if (!this.checkCollision(rotated, this.currentPiece.x + ox, this.currentPiece.y + oy)) {
        this.currentPiece.shape = rotated;
        this.currentPiece.x += ox;
        this.currentPiece.y += oy;
        this.sound.playRotate();

        if (this.isLocking) {
          this.lockDelayCounter = 0;
        }
        return;
      }
    }
  }

  // --- ソフトドロップ ---
  softDrop() {
    if (!this.isPlaying || this.isPaused || this.isGameOver || !this.currentPiece) return;

    if (!this.checkCollision(this.currentPiece.shape, this.currentPiece.x, this.currentPiece.y + 1)) {
      this.currentPiece.y += 1;
      this.score += 1;
      this.updateStatsUI();
      this.sound.playSoftDrop();
      this.dropCounter = 0;
    }
  }

  // --- ハードドロップ ---
  hardDrop() {
    if (!this.isPlaying || this.isPaused || this.isGameOver || !this.currentPiece) return;

    let dropDist = 0;
    while (!this.checkCollision(this.currentPiece.shape, this.currentPiece.x, this.currentPiece.y + 1)) {
      this.currentPiece.y += 1;
      dropDist += 1;
    }

    this.score += dropDist * 2;
    this.updateStatsUI();
    this.sound.playHardDrop();
    this.lockPiece();
  }

  // --- ゴースト位置計算 ---
  getGhostY() {
    if (!this.currentPiece) return 0;
    let ghostY = this.currentPiece.y;
    while (!this.checkCollision(this.currentPiece.shape, this.currentPiece.x, ghostY + 1)) {
      ghostY++;
    }
    return ghostY;
  }

  // --- ミノの固定 ---
  lockPiece() {
    const { shape, x, y, type } = this.currentPiece;

    for (let r = 0; r < shape.length; r++) {
      for (let c = 0; c < shape[r].length; c++) {
        if (shape[r][c] !== 0) {
          const boardY = y + r;
          const boardX = x + c;
          if (boardY >= 0 && boardY < ROWS && boardX >= 0 && boardX < COLS) {
            this.grid[boardY][boardX] = type;
          }
        }
      }
    }

    this.currentPiece = null;
    this.checkLines();
  }

  // --- ライン消去判定 ---
  checkLines() {
    const linesToClear = [];
    for (let r = ROWS - 1; r >= 0; r--) {
      if (this.grid[r].every(cell => cell !== 0)) {
        linesToClear.push(r);
      }
    }

    if (linesToClear.length > 0) {
      this.clearingLines = linesToClear;
      this.clearAnimationTimer = 180; // 180msのフラッシュアニメーション

      // サウンド再生
      this.sound.playClear(linesToClear.length);

      // パーティクル生成
      linesToClear.forEach(row => {
        for (let c = 0; c < COLS; c++) {
          const px = c * BLOCK_SIZE + BLOCK_SIZE / 2;
          const py = row * BLOCK_SIZE + BLOCK_SIZE / 2;
          const cellType = this.grid[row][c];
          const color = TETROMINOES[cellType]?.color || '#ffffff';
          this.particles.addSparkles(px, py, color, 4);
        }
      });

      // スコア & 通知
      const count = linesToClear.length;
      const basePts = SCORE_TABLE[count] || count * 200;
      this.score += basePts * this.level;
      this.lines += count;

      if (count === 4) {
        this.showBanner('TETRIS!', 'tetris');
      } else if (count === 3) {
        this.showBanner('TRIPLE!', 'clear');
      } else if (count === 2) {
        this.showBanner('DOUBLE!', 'clear');
      }

      // レベルアップ判定 (10ライン毎)
      const newLevel = Math.floor(this.lines / 10) + 1;
      if (newLevel > this.level) {
        this.level = newLevel;
        this.dropInterval = this.getSpeedForLevel(this.level);
        this.sound.playLevelUp();
        setTimeout(() => {
          this.showBanner(`LEVEL ${this.level}!`, 'levelup');
        }, count === 4 ? 600 : 0);
      }

      this.updateStatsUI();
    } else {
      this.spawnPiece();
    }
  }

  // --- 消去完了処理 ---
  finishLineClear() {
    this.clearingLines.sort((a, b) => a - b);
    for (const r of this.clearingLines) {
      this.grid.splice(r, 1);
      this.grid.unshift(Array(COLS).fill(0));
    }
    this.clearingLines = [];
    this.spawnPiece();
  }

  // --- ゲームオーバー ---
  gameOver() {
    this.isGameOver = true;
    this.isPlaying = false;
    this.sound.playGameOver();

    if (this.score > this.highScore) {
      this.highScore = this.score;
      localStorage.setItem('tetris_high_score', this.highScore.toString());
      this.updateStatsUI();
    }

    this.showOverlay(
      'GAME OVER',
      `最終スコア: ${this.score.toLocaleString()} 点\n消去ライン: ${this.lines} 行`,
      'TRY AGAIN'
    );
  }

  // --- UI更新 ---
  updateStatsUI() {
    this.scoreDisplay.textContent = this.score.toLocaleString();
    this.levelDisplay.textContent = this.level;
    this.linesDisplay.textContent = this.lines;
    this.highScoreDisplay.textContent = this.highScore.toLocaleString();
  }

  // --- イベントバインド ---
  bindEvents() {
    window.addEventListener('keydown', (e) => {
      // スペースキーや矢印キーでのブラウザスクロールを防止
      if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'Space', ' '].includes(e.key)) {
        e.preventDefault();
      }

      if (!this.isPlaying && (e.code === 'Space' || e.code === 'Enter')) {
        this.start();
        return;
      }

      if (e.code === 'KeyP' || e.key === 'Escape') {
        this.pause();
        return;
      }

      if (e.code === 'KeyR') {
        this.restart();
        return;
      }

      if (e.code === 'KeyM') {
        const isMuted = this.sound.toggleMute();
        this.soundIcon.textContent = isMuted ? '🔇' : '🔊';
        return;
      }

      if (!this.isPlaying || this.isPaused || this.isGameOver) return;

      // 初回キー押下
      if (!this.keys[e.code]) {
        this.keys[e.code] = true;
        this.handleKeyPress(e.code);
        this.dasTimers[e.code] = {
          startTime: performance.now(),
          lastRepeatTime: performance.now(),
          isRepeating: false
        };
      }
    });

    window.addEventListener('keyup', (e) => {
      this.keys[e.code] = false;
      delete this.dasTimers[e.code];
    });

    // ボタンUIイベント
    this.overlayActionBtn.addEventListener('click', () => {
      if (this.isPaused) {
        this.pause();
      } else {
        this.start();
      }
    });

    this.soundToggleBtn.addEventListener('click', () => {
      const isMuted = this.sound.toggleMute();
      this.soundIcon.textContent = isMuted ? '🔇' : '🔊';
    });

    this.pauseBtn.addEventListener('click', () => {
      this.pause();
    });

    this.restartBtn.addEventListener('click', () => {
      this.restart();
    });

    // バーチャルコントローラー（タッチ/クリック）
    this.bindVirtualControls();
  }

  bindVirtualControls() {
    const bindBtn = (id, action) => {
      const btn = document.getElementById(id);
      if (!btn) return;
      btn.addEventListener('click', (e) => {
        e.preventDefault();
        this.sound.init();
        if (!this.isPlaying) {
          this.start();
          return;
        }
        action();
      });
    };

    const bindRepeatBtn = (id, action) => {
      const btn = document.getElementById(id);
      if (!btn) return;
      let repeatTimer = null;
      let initialDelayTimer = null;

      const startAction = (e) => {
        e.preventDefault();
        this.sound.init();
        if (!this.isPlaying) {
          this.start();
          return;
        }
        action();
        initialDelayTimer = setTimeout(() => {
          repeatTimer = setInterval(action, 50);
        }, 160);
      };

      const stopAction = () => {
        clearTimeout(initialDelayTimer);
        clearInterval(repeatTimer);
      };

      btn.addEventListener('mousedown', startAction);
      btn.addEventListener('mouseup', stopAction);
      btn.addEventListener('mouseleave', stopAction);
      btn.addEventListener('touchstart', startAction, { passive: false });
      btn.addEventListener('touchend', stopAction);
      btn.addEventListener('touchcancel', stopAction);
    };

    bindBtn('vBtnHold', () => this.hold());
    bindBtn('vBtnRotL', () => this.rotate(-1));
    bindBtn('vBtnRotR', () => this.rotate(1));
    bindBtn('vBtnHard', () => this.hardDrop());

    bindRepeatBtn('vBtnLeft', () => this.move(-1));
    bindRepeatBtn('vBtnRight', () => this.move(1));
    bindRepeatBtn('vBtnDown', () => this.softDrop());
  }

  handleKeyPress(code) {
    switch (code) {
      case 'ArrowLeft':
        this.move(-1);
        break;
      case 'ArrowRight':
        this.move(1);
        break;
      case 'ArrowUp':
      case 'KeyX':
        this.rotate(1); // 右回転
        break;
      case 'KeyZ':
        this.rotate(-1); // 左回転
        break;
      case 'ArrowDown':
        this.softDrop();
        break;
      case 'Space':
        this.hardDrop();
        break;
      case 'KeyC':
      case 'ShiftLeft':
      case 'ShiftRight':
        this.hold();
        break;
    }
  }

  // 長押し（DAS/ARR）のフレーム処理
  processKeyRepeats(now) {
    if (!this.isPlaying || this.isPaused || this.isGameOver) return;

    for (const code of ['ArrowLeft', 'ArrowRight', 'ArrowDown']) {
      if (this.keys[code] && this.dasTimers[code]) {
        const timer = this.dasTimers[code];
        const elapsed = now - timer.startTime;

        if (!timer.isRepeating) {
          if (elapsed >= this.DAS_DELAY) {
            timer.isRepeating = true;
            timer.lastRepeatTime = now;
            this.handleKeyPress(code);
          }
        } else {
          if (now - timer.lastRepeatTime >= this.ARR_RATE) {
            timer.lastRepeatTime = now;
            this.handleKeyPress(code);
          }
        }
      }
    }
  }

  // --- メインゲームループ ---
  gameLoop(time = 0) {
    const deltaTime = time - this.lastTime;
    this.lastTime = time;

    if (this.isPlaying && !this.isPaused && !this.isGameOver) {
      this.processKeyRepeats(time);

      // ライン消去中アニメーション
      if (this.clearingLines.length > 0) {
        this.clearAnimationTimer -= deltaTime;
        if (this.clearAnimationTimer <= 0) {
          this.finishLineClear();
        }
      } else if (this.currentPiece) {
        // 自然落下
        this.dropCounter += deltaTime;
        if (this.dropCounter > this.dropInterval) {
          if (!this.checkCollision(this.currentPiece.shape, this.currentPiece.x, this.currentPiece.y + 1)) {
            this.currentPiece.y++;
            this.isLocking = false;
            this.lockDelayCounter = 0;
          } else {
            this.isLocking = true;
          }
          this.dropCounter = 0;
        }

        // 接地ロック猶予（Lock Delay）
        if (this.checkCollision(this.currentPiece.shape, this.currentPiece.x, this.currentPiece.y + 1)) {
          this.isLocking = true;
          this.lockDelayCounter += deltaTime;
          if (this.lockDelayCounter >= this.lockDelayLimit) {
            this.lockPiece();
          }
        } else {
          this.isLocking = false;
          this.lockDelayCounter = 0;
        }
      }

      this.particles.update();
    }

    this.draw();

    if (this.isPlaying && !this.isPaused) {
      requestAnimationFrame(this.gameLoop.bind(this));
    }
  }

  // --- 描画処理 ---
  draw() {
    // 1. メイン盤面のクリア
    this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

    // グリッド線描画
    this.drawGridLines(this.ctx, COLS, ROWS, BLOCK_SIZE);

    // 固定された盤面ブロックの描画
    for (let r = 0; r < ROWS; r++) {
      const isClearing = this.clearingLines.includes(r);
      for (let c = 0; c < COLS; c++) {
        const type = this.grid[r][c];
        if (type !== 0) {
          if (isClearing) {
            // 消去フラッシュ
            this.drawFlashBlock(this.ctx, c * BLOCK_SIZE, r * BLOCK_SIZE, BLOCK_SIZE);
          } else {
            this.drawBlock(this.ctx, c * BLOCK_SIZE, r * BLOCK_SIZE, BLOCK_SIZE, type);
          }
        }
      }
    }

    // ゴーストブロック & 操作中ブロック描画
    if (this.currentPiece && this.clearingLines.length === 0) {
      const ghostY = this.getGhostY();
      // ゴーストブロック
      this.drawGhostPiece(this.ctx, this.currentPiece, ghostY);
      // カレントピース
      this.drawPiece(this.ctx, this.currentPiece);
    }

    // パーティクル描画
    this.particles.draw(this.ctx);

    // 2. HOLD パネルの描画
    this.drawHoldPanel();

    // 3. NEXT パネルの描画
    this.drawNextPanel();
  }

  drawGridLines(ctx, cols, rows, size) {
    ctx.save();
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.04)';
    ctx.lineWidth = 1;

    for (let c = 0; c <= cols; c++) {
      ctx.beginPath();
      ctx.moveTo(c * size, 0);
      ctx.lineTo(c * size, rows * size);
      ctx.stroke();
    }
    for (let r = 0; r <= rows; r++) {
      ctx.beginPath();
      ctx.moveTo(0, r * size);
      ctx.lineTo(cols * size, r * size);
      ctx.stroke();
    }
    ctx.restore();
  }

  // 1マスのブロックをリッチに描画（ハイライト・立体感）
  drawBlock(ctx, x, y, size, type, alpha = 1.0) {
    const config = TETROMINOES[type];
    if (!config) return;

    ctx.save();
    ctx.globalAlpha = alpha;

    // 基本背景
    ctx.fillStyle = config.color;
    ctx.fillRect(x + 1, y + 1, size - 2, size - 2);

    // 上辺 & 左辺のハイライト（明）
    ctx.fillStyle = config.light;
    ctx.beginPath();
    ctx.moveTo(x + 1, y + 1);
    ctx.lineTo(x + size - 1, y + 1);
    ctx.lineTo(x + size - 4, y + 4);
    ctx.lineTo(x + 4, y + 4);
    ctx.lineTo(x + 4, y + size - 4);
    ctx.lineTo(x + 1, y + size - 1);
    ctx.closePath();
    ctx.fill();

    // 下辺 & 右辺のシャドウ（暗）
    ctx.fillStyle = config.dark;
    ctx.beginPath();
    ctx.moveTo(x + size - 1, y + 1);
    ctx.lineTo(x + size - 1, y + size - 1);
    ctx.lineTo(x + 1, y + size - 1);
    ctx.lineTo(x + 4, y + size - 4);
    ctx.lineTo(x + size - 4, y + size - 4);
    ctx.lineTo(x + size - 4, y + 4);
    ctx.closePath();
    ctx.fill();

    // 中心のアクセントグラデーション
    const grad = ctx.createLinearGradient(x + 4, y + 4, x + size - 4, y + size - 4);
    grad.addColorStop(0, 'rgba(255, 255, 255, 0.25)');
    grad.addColorStop(1, 'rgba(0, 0, 0, 0.15)');
    ctx.fillStyle = grad;
    ctx.fillRect(x + 4, y + 4, size - 8, size - 8);

    ctx.restore();
  }

  // 消去時のホワイトフラッシュブロック
  drawFlashBlock(ctx, x, y, size) {
    ctx.save();
    ctx.fillStyle = '#ffffff';
    ctx.shadowColor = '#00f0f0';
    ctx.shadowBlur = 15;
    ctx.fillRect(x + 1, y + 1, size - 2, size - 2);
    ctx.restore();
  }

  // ゴーストブロック（半透明 & 枠線）
  drawGhostPiece(ctx, piece, ghostY) {
    const { shape, x, type } = piece;
    const config = TETROMINOES[type];
    ctx.save();
    for (let r = 0; r < shape.length; r++) {
      for (let c = 0; c < shape[r].length; c++) {
        if (shape[r][c] !== 0) {
          const px = (x + c) * BLOCK_SIZE;
          const py = (ghostY + r) * BLOCK_SIZE;

          ctx.fillStyle = config.color;
          ctx.globalAlpha = 0.18;
          ctx.fillRect(px + 2, py + 2, BLOCK_SIZE - 4, BLOCK_SIZE - 4);

          ctx.strokeStyle = config.color;
          ctx.lineWidth = 1.5;
          ctx.globalAlpha = 0.55;
          ctx.strokeRect(px + 1.5, py + 1.5, BLOCK_SIZE - 3, BLOCK_SIZE - 3);
        }
      }
    }
    ctx.restore();
  }

  drawPiece(ctx, piece) {
    const { shape, x, y, type } = piece;
    for (let r = 0; r < shape.length; r++) {
      for (let c = 0; c < shape[r].length; c++) {
        if (shape[r][c] !== 0) {
          this.drawBlock(ctx, (x + c) * BLOCK_SIZE, (y + r) * BLOCK_SIZE, BLOCK_SIZE, type);
        }
      }
    }
  }

  // HOLD サブキャンバス描画
  drawHoldPanel() {
    this.holdCtx.clearRect(0, 0, this.holdCanvas.width, this.holdCanvas.height);
    if (!this.holdPieceType) return;

    const shape = TETROMINOES[this.holdPieceType].shape;
    const miniSize = 22;
    const offsetX = (this.holdCanvas.width - shape[0].length * miniSize) / 2;
    const offsetY = (this.holdCanvas.height - shape.length * miniSize) / 2;

    const alpha = this.canHold ? 1.0 : 0.35; // ホールド済みなら少し暗く

    for (let r = 0; r < shape.length; r++) {
      for (let c = 0; c < shape[r].length; c++) {
        if (shape[r][c] !== 0) {
          this.drawBlock(
            this.holdCtx,
            offsetX + c * miniSize,
            offsetY + r * miniSize,
            miniSize,
            this.holdPieceType,
            alpha
          );
        }
      }
    }
  }

  // NEXT サブキャンバス描画（次に来る3つ）
  drawNextPanel() {
    this.nextCtx.clearRect(0, 0, this.nextCanvas.width, this.nextCanvas.height);

    const miniSize = 20;
    const previewCount = 3;
    const slotHeight = this.nextCanvas.height / previewCount;

    for (let i = 0; i < previewCount; i++) {
      const type = this.nextQueue[i];
      if (!type) continue;

      const shape = TETROMINOES[type].shape;
      const offsetX = (this.nextCanvas.width - shape[0].length * miniSize) / 2;
      const offsetY = i * slotHeight + (slotHeight - shape.length * miniSize) / 2;

      for (let r = 0; r < shape.length; r++) {
        for (let c = 0; c < shape[r].length; c++) {
          if (shape[r][c] !== 0) {
            this.drawBlock(
              this.nextCtx,
              offsetX + c * miniSize,
              offsetY + r * miniSize,
              miniSize,
              type
            );
          }
        }
      }

      // スロット境界線
      if (i < previewCount - 1) {
        this.nextCtx.save();
        this.nextCtx.strokeStyle = 'rgba(255, 255, 255, 0.06)';
        this.nextCtx.beginPath();
        this.nextCtx.moveTo(15, (i + 1) * slotHeight);
        this.nextCtx.lineTo(this.nextCanvas.width - 15, (i + 1) * slotHeight);
        this.nextCtx.stroke();
        this.nextCtx.restore();
      }
    }
  }
}

// ゲーム起動
window.addEventListener('DOMContentLoaded', () => {
  window.game = new TetrisGame();
});
