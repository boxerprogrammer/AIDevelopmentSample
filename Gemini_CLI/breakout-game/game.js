/**
 * NEO BREAKOUT - Retro-futuristic Synthwave Breakout Game
 * Vanilla JavaScript & HTML5 Canvas
 * Features:
 * - Web Audio API procedural sound synthesis
 * - Item Drop System (Multi-Ball, Expand Paddle, Piercing Ball, Slow Ball)
 * - 3-Stage System with distinct layouts, indestructible metal blocks & scaling difficulty
 * - LocalStorage-based Top 5 Hall of Fame High Score Ranking
 */

// ==========================================
// 1. サウンドマネージャー (Web Audio API)
// ==========================================
class SoundManager {
  constructor() {
    this.ctx = null;
    this.enabled = true;
  }

  init() {
    if (!this.ctx) {
      const AudioContextClass = window.AudioContext || window.webkitAudioContext;
      if (AudioContextClass) {
        this.ctx = new AudioContextClass();
      }
    }
    if (this.ctx && this.ctx.state === 'suspended') {
      this.ctx.resume();
    }
  }

  // 1. パドル反射音: 高めの短いピコピコ音（Triangle波, 440Hz -> 880Hz）
  paddleHit() {
    this.init();
    if (!this.ctx) return;
    try {
      const t = this.ctx.currentTime;
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();

      osc.type = 'triangle';
      osc.frequency.setValueAtTime(440, t);
      osc.frequency.exponentialRampToValueAtTime(880, t + 0.07);

      gain.gain.setValueAtTime(0.25, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + 0.07);

      osc.connect(gain);
      gain.connect(this.ctx.destination);

      osc.start(t);
      osc.stop(t + 0.07);
    } catch (e) {}
  }

  // 2. ブロック破壊音: 爽快感のあるポップ音（Square波, 300Hz -> 150Hz）
  brickBreak() {
    this.init();
    if (!this.ctx) return;
    try {
      const t = this.ctx.currentTime;
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();

      osc.type = 'square';
      osc.frequency.setValueAtTime(300, t);
      osc.frequency.exponentialRampToValueAtTime(150, t + 0.09);

      gain.gain.setValueAtTime(0.22, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + 0.09);

      osc.connect(gain);
      gain.connect(this.ctx.destination);

      osc.start(t);
      osc.stop(t + 0.09);
    } catch (e) {}
  }

  // 耐久ブロックヒット音（ダメージ時）
  brickHit() {
    this.init();
    if (!this.ctx) return;
    try {
      const t = this.ctx.currentTime;
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();

      osc.type = 'sine';
      osc.frequency.setValueAtTime(420, t);
      osc.frequency.exponentialRampToValueAtTime(280, t + 0.06);

      gain.gain.setValueAtTime(0.18, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + 0.06);

      osc.connect(gain);
      gain.connect(this.ctx.destination);

      osc.start(t);
      osc.stop(t + 0.06);
    } catch (e) {}
  }

  // メタルブロック（破壊不能）衝突音: 硬質な金属音
  metalHit() {
    this.init();
    if (!this.ctx) return;
    try {
      const t = this.ctx.currentTime;
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();

      osc.type = 'sine';
      osc.frequency.setValueAtTime(1200, t);
      osc.frequency.exponentialRampToValueAtTime(600, t + 0.1);

      gain.gain.setValueAtTime(0.28, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + 0.1);

      osc.connect(gain);
      gain.connect(this.ctx.destination);

      osc.start(t);
      osc.stop(t + 0.1);
    } catch (e) {}
  }

  // 3. 特殊アイテム取得音: 上昇アルペジオ風ファンファーレ音
  itemCollect() {
    this.init();
    if (!this.ctx) return;
    try {
      const baseTime = this.ctx.currentTime;
      const arpeggio = [523.25, 659.25, 783.99, 1046.50]; // C5, E5, G5, C6
      arpeggio.forEach((freq, i) => {
        const t = baseTime + i * 0.055;
        const osc = this.ctx.createOscillator();
        const gain = this.ctx.createGain();

        osc.type = 'triangle';
        osc.frequency.setValueAtTime(freq, t);

        gain.gain.setValueAtTime(0.22, t);
        gain.gain.exponentialRampToValueAtTime(0.001, t + 0.12);

        osc.connect(gain);
        gain.connect(this.ctx.destination);

        osc.start(t);
        osc.stop(t + 0.12);
      });
    } catch (e) {}
  }

  // 4. ボール落下/ミス音: 低音の下降ディストーション音（Sawtooth波, 200Hz -> 60Hz）
  lifeLost() {
    this.init();
    if (!this.ctx) return;
    try {
      const t = this.ctx.currentTime;
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();

      osc.type = 'sawtooth';
      osc.frequency.setValueAtTime(200, t);
      osc.frequency.linearRampToValueAtTime(60, t + 0.35);

      gain.gain.setValueAtTime(0.32, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + 0.35);

      osc.connect(gain);
      gain.connect(this.ctx.destination);

      osc.start(t);
      osc.stop(t + 0.35);
    } catch (e) {}
  }

  // 壁衝突音
  wallHit() {
    this.init();
    if (!this.ctx) return;
    try {
      const t = this.ctx.currentTime;
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();
      osc.type = 'sine';
      osc.frequency.setValueAtTime(240, t);
      osc.frequency.exponentialRampToValueAtTime(120, t + 0.05);

      gain.gain.setValueAtTime(0.12, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + 0.05);

      osc.connect(gain);
      gain.connect(this.ctx.destination);
      osc.start(t);
      osc.stop(t + 0.05);
    } catch (e) {}
  }

  // ステージクリア音（短いファンファーレ）
  stageClear() {
    this.init();
    if (!this.ctx) return;
    try {
      const baseTime = this.ctx.currentTime;
      const notes = [523.25, 659.25, 783.99, 1046.50, 1318.51];
      notes.forEach((f, i) => {
        const t = baseTime + i * 0.08;
        const osc = this.ctx.createOscillator();
        const gain = this.ctx.createGain();
        osc.type = 'triangle';
        osc.frequency.setValueAtTime(f, t);
        gain.gain.setValueAtTime(0.22, t);
        gain.gain.exponentialRampToValueAtTime(0.001, t + 0.2);
        osc.connect(gain);
        gain.connect(this.ctx.destination);
        osc.start(t);
        osc.stop(t + 0.2);
      });
    } catch (e) {}
  }

  // ゲームオーバー音
  gameOver() {
    this.init();
    if (!this.ctx) return;
    const notes = [220, 196, 174, 130];
    notes.forEach((note, i) => {
      setTimeout(() => {
        if (!this.ctx) return;
        const t = this.ctx.currentTime;
        const osc = this.ctx.createOscillator();
        const gain = this.ctx.createGain();
        osc.type = 'sawtooth';
        osc.frequency.setValueAtTime(note, t);
        gain.gain.setValueAtTime(0.25, t);
        gain.gain.exponentialRampToValueAtTime(0.001, t + 0.22);
        osc.connect(gain);
        gain.connect(this.ctx.destination);
        osc.start(t);
        osc.stop(t + 0.22);
      }, i * 150);
    });
  }

  // 5. 全クリ祝勝音: 華やかなメロディコード
  victory() {
    this.init();
    if (!this.ctx) return;
    const baseTime = this.ctx.currentTime;
    const chords = [
      { notes: [440, 554.37, 659.25], time: 0, dur: 0.22 },
      { notes: [493.88, 622.25, 739.99], time: 0.2, dur: 0.22 },
      { notes: [554.37, 659.25, 830.61], time: 0.4, dur: 0.25 },
      { notes: [659.25, 830.61, 987.77, 1318.51], time: 0.65, dur: 0.9 }
    ];

    chords.forEach(c => {
      c.notes.forEach(f => {
        const t = baseTime + c.time;
        const osc = this.ctx.createOscillator();
        const gain = this.ctx.createGain();
        osc.type = 'triangle';
        osc.frequency.setValueAtTime(f, t);
        gain.gain.setValueAtTime(0.18, t);
        gain.gain.exponentialRampToValueAtTime(0.001, t + c.dur);
        osc.connect(gain);
        gain.connect(this.ctx.destination);
        osc.start(t);
        osc.stop(t + c.dur);
      });
    });
  }
}

// ==========================================
// 2. ハイスコアランキング管理 (LocalStorage)
// ==========================================
class LeaderboardManager {
  constructor() {
    this.STORAGE_KEY = 'neo_breakout_rankings_v2';
    this.DEFAULT_RANKINGS = [
      { name: 'CYBER', score: 12000, stage: 'STAGE 3', date: '2026/09/01' },
      { name: 'NEO', score: 9500, stage: 'STAGE 3', date: '2026/09/04' },
      { name: 'PIXEL', score: 6800, stage: 'STAGE 2', date: '2026/09/07' },
      { name: 'RETRO', score: 4200, stage: 'STAGE 2', date: '2026/09/08' },
      { name: 'ROOKIE', score: 2000, stage: 'STAGE 1', date: '2026/09/10' }
    ];
  }

  getRankings() {
    try {
      const data = localStorage.getItem(this.STORAGE_KEY);
      if (data) {
        const parsed = JSON.parse(data);
        if (Array.isArray(parsed) && parsed.length > 0) {
          return parsed;
        }
      }
    } catch (e) {}
    return [...this.DEFAULT_RANKINGS];
  }

  saveRankings(list) {
    try {
      localStorage.setItem(this.STORAGE_KEY, JSON.stringify(list.slice(0, 5)));
    } catch (e) {}
  }

  isTopScore(score) {
    if (score <= 0) return false;
    const list = this.getRankings();
    if (list.length < 5) return true;
    return score > list[list.length - 1].score;
  }

  addRecord(name, score, stageText) {
    const list = this.getRankings();
    const now = new Date();
    const dateStr = `${now.getFullYear()}/${String(now.getMonth() + 1).padStart(2, '0')}/${String(now.getDate()).padStart(2, '0')}`;
    list.push({
      name: (name || 'PLAYER').trim().toUpperCase().slice(0, 8),
      score: score,
      stage: stageText,
      date: dateStr
    });
    list.sort((a, b) => b.score - a.score);
    const top5 = list.slice(0, 5);
    this.saveRankings(top5);
    return top5;
  }

  reset() {
    try {
      localStorage.removeItem(this.STORAGE_KEY);
    } catch (e) {}
    return [...this.DEFAULT_RANKINGS];
  }

  getHighestScore() {
    const list = this.getRankings();
    return list.length > 0 ? list[0].score : 0;
  }
}

// ==========================================
// 3. ゲーム定数 & ステージ定義 (全3ステージ)
// ==========================================
const CANVAS_WIDTH = 800;
const CANVAS_HEIGHT = 600;
const BASE_PADDLE_WIDTH = 110;
const BASE_PADDLE_HEIGHT = 14;

const GAME_STATE = {
  START: 'START',
  PLAYING: 'PLAYING',
  BALL_READY: 'BALL_READY',
  STAGE_CLEAR: 'STAGE_CLEAR',
  GAMEOVER: 'GAMEOVER',
  ALL_CLEAR: 'ALL_CLEAR'
};

// ステージレイアウト定義
// 0: 空白, 1: 1耐久, 2: 2耐久, 3: 3耐久, 'M': 破壊不能メタルブロック
const STAGE_CONFIGS = [
  {
    stage: 1,
    name: 'STAGE 1: STANDARD MATRIX',
    speedMultiplier: 1.0,
    baseSpeed: 7.0,
    layout: [
      [1, 1, 1, 1, 1, 1, 1, 1],
      [1, 2, 1, 1, 1, 1, 2, 1],
      [1, 1, 2, 2, 2, 2, 1, 1],
      [1, 1, 1, 1, 1, 1, 1, 1],
      [0, 1, 1, 1, 1, 1, 1, 0]
    ]
  },
  {
    stage: 2,
    name: 'STAGE 2: CHECKER PYRAMID',
    speedMultiplier: 1.1,
    baseSpeed: 7.7,
    layout: [
      [0, 0, 0, 2, 2, 0, 0, 0],
      [0, 0, 2, 1, 1, 2, 0, 0],
      [0, 2, 1, 2, 2, 1, 2, 0],
      [2, 1, 2, 1, 1, 2, 1, 2],
      [1, 2, 1, 2, 2, 1, 2, 1],
      [0, 2, 0, 2, 2, 0, 2, 0]
    ]
  },
  {
    stage: 3,
    name: 'STAGE 3: NEON FORTRESS',
    speedMultiplier: 1.25,
    baseSpeed: 8.75,
    layout: [
      ['M', 3, 3, 'M', 'M', 3, 3, 'M'],
      [ 3,  0, 3,  2,   2,  3, 0,  3 ],
      ['M', 0, 'M', 3,  3, 'M', 0, 'M'],
      [ 3,  3,  0, 'M', 'M', 0, 3,  3 ],
      [ 0, 'M', 3,  3,   3,  3, 'M', 0]
    ]
  }
];

const ITEM_TYPES = {
  MULTI_BALL: {
    id: 'MULTI_BALL',
    name: 'MULTI-BALL',
    symbol: '●×3',
    color: '#00f0ff',
    glow: '#00f0ff',
    textColor: '#080710'
  },
  EXPAND_PADDLE: {
    id: 'EXPAND_PADDLE',
    name: 'EXPAND PADDLE',
    symbol: '◀▶',
    color: '#00ff88',
    glow: '#00ff88',
    duration: 10,
    textColor: '#080710'
  },
  PIERCING_BALL: {
    id: 'PIERCING_BALL',
    name: 'PIERCING BALL',
    symbol: '⚡',
    color: '#ff2255',
    glow: '#ff2255',
    duration: 8,
    textColor: '#ffffff'
  },
  SLOW_BALL: {
    id: 'SLOW_BALL',
    name: 'SLOW BALL',
    symbol: '▼',
    color: '#ffe600',
    glow: '#ffe600',
    duration: 10,
    textColor: '#080710'
  }
};

// ==========================================
// 4. ゲームメインクラス
// ==========================================
class NeonBreakoutGame {
  constructor() {
    this.canvas = document.getElementById('gameCanvas');
    this.ctx = this.canvas.getContext('2d');

    // UI要素
    this.stageDisplayEl = document.getElementById('stage-display');
    this.scoreEl = document.getElementById('score');
    this.highScoreEl = document.getElementById('high-score');
    this.livesEl = document.getElementById('lives');
    this.powerupBannerEl = document.getElementById('powerupBanner');

    // モーダル要素
    this.rankingBtn = document.getElementById('rankingBtn');
    this.rankingModal = document.getElementById('rankingModal');
    this.closeRankingBtn = document.getElementById('closeRankingBtn');
    this.closeRankingBottomBtn = document.getElementById('closeRankingBottomBtn');
    this.resetRankingBtn = document.getElementById('resetRankingBtn');
    this.rankingTableBody = document.getElementById('rankingTableBody');

    this.nameInputModal = document.getElementById('nameInputModal');
    this.nameInputForm = document.getElementById('nameInputForm');
    this.playerNameInput = document.getElementById('playerNameInput');
    this.recordScoreEl = document.getElementById('recordScore');
    this.recordStageEl = document.getElementById('recordStage');

    this.sound = new SoundManager();
    this.leaderboard = new LeaderboardManager();

    this.canvas.width = CANVAS_WIDTH;
    this.canvas.height = CANVAS_HEIGHT;

    this.highScore = this.leaderboard.getHighestScore();
    this.highScoreEl.textContent = this.formatScore(this.highScore);

    this.keys = { left: false, right: false };

    // ゲーム状態
    this.currentStageIndex = 0; // 0, 1, 2
    this.state = GAME_STATE.START;
    this.score = 0;
    this.lives = 3;
    this.combo = 0;

    // ステージクリア遷移演出用
    this.stageClearTimer = 0;
    this.stageClearBonus = 0;

    // パドル
    this.paddle = {
      width: BASE_PADDLE_WIDTH,
      height: BASE_PADDLE_HEIGHT,
      x: (CANVAS_WIDTH - BASE_PADDLE_WIDTH) / 2,
      y: CANVAS_HEIGHT - 40,
      speed: 8.5,
      color: '#00f0ff'
    };

    // ボール群
    this.balls = [];

    // パワーアップ状態
    this.powerups = {
      expand: { active: false, timer: 0, maxDuration: 10 },
      pierce: { active: false, timer: 0, maxDuration: 8 },
      slow: { active: false, timer: 0, maxDuration: 10 }
    };

    this.fallingItems = [];
    this.bricks = [];
    this.particles = [];
    this.stars = this.generateStars(60);

    this.lastTime = 0;
    this.blinkTimer = 0;
    this.isModalOpen = false;

    this.initEventListeners();
    this.initModalEvents();
    this.loadStage(0);
    this.initBallOnPaddle();
    this.updateUI();

    requestAnimationFrame((ts) => this.gameLoop(ts));
  }

  generateStars(count) {
    const stars = [];
    for (let i = 0; i < count; i++) {
      stars.push({
        x: Math.random() * CANVAS_WIDTH,
        y: Math.random() * CANVAS_HEIGHT,
        size: Math.random() * 2 + 0.5,
        alpha: Math.random() * 0.7 + 0.3,
        speed: Math.random() * 0.3 + 0.1
      });
    }
    return stars;
  }

  formatScore(num) {
    return Math.max(0, num).toString().padStart(5, '0');
  }

  updateUI() {
    this.stageDisplayEl.textContent = `STAGE ${this.currentStageIndex + 1}/3`;
    this.scoreEl.textContent = this.formatScore(this.score);
    this.highScoreEl.textContent = this.formatScore(Math.max(this.highScore, this.score));
    this.livesEl.textContent = '❤'.repeat(Math.max(0, this.lives));
    this.updatePowerupBanner();
  }

  updatePowerupBanner() {
    if (!this.powerupBannerEl) return;
    const activeList = [];

    if (this.powerups.expand.active) {
      activeList.push({
        name: 'EXPAND',
        color: ITEM_TYPES.EXPAND_PADDLE.color,
        timer: this.powerups.expand.timer,
        max: this.powerups.expand.maxDuration
      });
    }
    if (this.powerups.pierce.active) {
      activeList.push({
        name: 'PIERCE',
        color: ITEM_TYPES.PIERCING_BALL.color,
        timer: this.powerups.pierce.timer,
        max: this.powerups.pierce.maxDuration
      });
    }
    if (this.powerups.slow.active) {
      activeList.push({
        name: 'SLOW',
        color: ITEM_TYPES.SLOW_BALL.color,
        timer: this.powerups.slow.timer,
        max: this.powerups.slow.maxDuration
      });
    }

    if (activeList.length === 0) {
      this.powerupBannerEl.innerHTML = '';
      return;
    }

    this.powerupBannerEl.innerHTML = activeList.map(item => {
      const pct = Math.max(0, Math.min(100, (item.timer / item.max) * 100));
      return `
        <div class="powerup-pill" style="--border-c: ${item.color}; --text-c: ${item.color};">
          <span>${item.name} (${item.timer.toFixed(1)}s)</span>
          <div class="gauge">
            <div class="gauge-fill" style="width: ${pct}%; --bar-c: ${item.color};"></div>
          </div>
        </div>
      `;
    }).join('');
  }

  // ==========================================
  // 5. ステージ管理
  // ==========================================
  loadStage(stageIndex) {
    this.currentStageIndex = stageIndex;
    const config = STAGE_CONFIGS[stageIndex];
    const layout = config.layout;

    const rows = layout.length;
    const cols = layout[0].length;
    const padding = 8;
    const offsetTop = 60;
    const offsetLeft = 35;
    const brickW = (CANVAS_WIDTH - (offsetLeft * 2) - (padding * (cols - 1))) / cols;
    const brickH = 22;

    this.bricks = [];

    for (let r = 0; r < rows; r++) {
      this.bricks[r] = [];
      for (let c = 0; c < cols; c++) {
        const val = layout[r][c];
        if (val === 0) {
          this.bricks[r][c] = null;
          continue;
        }

        const brickX = offsetLeft + c * (brickW + padding);
        const brickY = offsetTop + r * (brickH + padding);

        if (val === 'M') {
          // 破壊不能メタルブロック
          this.bricks[r][c] = {
            id: `${stageIndex}-${r}-${c}`,
            x: brickX,
            y: brickY,
            width: brickW,
            height: brickH,
            indestructible: true,
            hp: Infinity,
            maxHp: Infinity,
            color: '#b8c4d8',
            glow: '#ffffff',
            points: 0,
            alive: true
          };
        } else {
          // 通常ブロック（耐久1〜3）
          const hp = parseInt(val, 10);
          const colorData = this.getBrickColorByHp(hp);
          this.bricks[r][c] = {
            id: `${stageIndex}-${r}-${c}`,
            x: brickX,
            y: brickY,
            width: brickW,
            height: brickH,
            indestructible: false,
            hp: hp,
            maxHp: hp,
            color: colorData.color,
            glow: colorData.glow,
            points: hp * 15,
            alive: true
          };
        }
      }
    }
  }

  getBrickColorByHp(hp) {
    if (hp >= 3) return { color: '#ff007f', glow: '#ff007f' }; // マゼンタ
    if (hp === 2) return { color: '#ff7700', glow: '#ff9933' }; // オレンジ
    return { color: '#00f0ff', glow: '#00f0ff' };               // シアン
  }

  initBallOnPaddle() {
    const config = STAGE_CONFIGS[this.currentStageIndex];
    this.balls = [{
      x: this.paddle.x + this.paddle.width / 2,
      y: this.paddle.y - 8,
      radius: 7,
      baseSpeed: config.baseSpeed,
      vx: 0,
      vy: 0,
      trail: [],
      piercedSet: new Set()
    }];
    this.combo = 0;
  }

  launchBall() {
    this.state = GAME_STATE.PLAYING;
    if (this.balls.length === 0) {
      this.initBallOnPaddle();
    }
    const angle = (Math.random() * 40 - 20) * (Math.PI / 180);
    const speed = this.balls[0].baseSpeed;
    this.balls[0].vx = speed * Math.sin(angle);
    this.balls[0].vy = -speed * Math.cos(angle);
    this.sound.paddleHit();
  }

  restartGame() {
    this.score = 0;
    this.lives = 3;
    this.combo = 0;
    this.particles = [];
    this.fallingItems = [];
    this.clearPowerups();
    this.paddle.width = BASE_PADDLE_WIDTH;
    this.paddle.x = (CANVAS_WIDTH - this.paddle.width) / 2;
    this.loadStage(0);
    this.initBallOnPaddle();
    this.updateUI();
    this.state = GAME_STATE.START;
  }

  clearPowerups() {
    this.powerups.expand.active = false;
    this.powerups.expand.timer = 0;
    this.powerups.pierce.active = false;
    this.powerups.pierce.timer = 0;
    this.powerups.slow.active = false;
    this.powerups.slow.timer = 0;
    this.paddle.width = BASE_PADDLE_WIDTH;
    this.clampPaddle();
  }

  // ==========================================
  // 6. 入力 & モーダルイベント
  // ==========================================
  initEventListeners() {
    const unlockAudio = () => this.sound.init();

    window.addEventListener('keydown', (e) => {
      unlockAudio();
      if (this.isModalOpen) return; // モーダル表示中はゲームキー無効

      if (e.code === 'ArrowLeft' || e.code === 'KeyA') {
        this.keys.left = true;
      }
      if (e.code === 'ArrowRight' || e.code === 'KeyD') {
        this.keys.right = true;
      }
      if (e.code === 'Space') {
        e.preventDefault();
        this.handleActionInput();
      }
    });

    window.addEventListener('keyup', (e) => {
      if (e.code === 'ArrowLeft' || e.code === 'KeyA') {
        this.keys.left = false;
      }
      if (e.code === 'ArrowRight' || e.code === 'KeyD') {
        this.keys.right = false;
      }
    });

    const getCanvasMouseX = (e) => {
      const rect = this.canvas.getBoundingClientRect();
      const scaleX = this.canvas.width / rect.width;
      return (e.clientX - rect.left) * scaleX;
    };

    this.canvas.addEventListener('mousemove', (e) => {
      if (this.isModalOpen) return;
      const mouseX = getCanvasMouseX(e);
      this.paddle.x = mouseX - this.paddle.width / 2;
      this.clampPaddle();

      if (this.state === GAME_STATE.START || this.state === GAME_STATE.BALL_READY) {
        if (this.balls.length > 0) {
          this.balls[0].x = this.paddle.x + this.paddle.width / 2;
        }
      }
    });

    this.canvas.addEventListener('click', () => {
      unlockAudio();
      if (this.isModalOpen) return;
      this.handleActionInput();
    });

    this.canvas.addEventListener('touchmove', (e) => {
      if (this.isModalOpen) return;
      e.preventDefault();
      if (e.touches.length > 0) {
        const mouseX = getCanvasMouseX(e.touches[0]);
        this.paddle.x = mouseX - this.paddle.width / 2;
        this.clampPaddle();
        if (this.state === GAME_STATE.START || this.state === GAME_STATE.BALL_READY) {
          if (this.balls.length > 0) {
            this.balls[0].x = this.paddle.x + this.paddle.width / 2;
          }
        }
      }
    }, { passive: false });

    this.canvas.addEventListener('touchstart', (e) => {
      if (this.isModalOpen) return;
      e.preventDefault();
      unlockAudio();
      this.handleActionInput();
    }, { passive: false });
  }

  initModalEvents() {
    // ランキング開く
    this.rankingBtn.addEventListener('click', () => {
      this.openRankingModal();
    });

    // ランキング閉じる
    this.closeRankingBtn.addEventListener('click', () => {
      this.closeRankingModal();
    });
    this.closeRankingBottomBtn.addEventListener('click', () => {
      this.closeRankingModal();
    });

    // ランキングリセット
    this.resetRankingBtn.addEventListener('click', () => {
      if (window.confirm('ハイスコアランキングデータを初期化してもよろしいですか？')) {
        this.leaderboard.reset();
        this.highScore = this.leaderboard.getHighestScore();
        this.updateUI();
        this.renderRankingTable();
      }
    });

    // 名前入力フォーム送信
    this.nameInputForm.addEventListener('submit', (e) => {
      e.preventDefault();
      const playerName = this.playerNameInput.value.trim() || 'PLAYER';
      const stageText = this.state === GAME_STATE.ALL_CLEAR 
        ? 'ALL CLEAR' 
        : `STAGE ${this.currentStageIndex + 1}`;

      this.leaderboard.addRecord(playerName, this.score, stageText);
      this.highScore = this.leaderboard.getHighestScore();
      this.updateUI();

      this.nameInputModal.classList.add('hidden');
      this.openRankingModal();
    });
  }

  openRankingModal() {
    this.isModalOpen = true;
    this.renderRankingTable();
    this.rankingModal.classList.remove('hidden');
  }

  closeRankingModal() {
    this.rankingModal.classList.add('hidden');
    this.isModalOpen = false;
  }

  renderRankingTable() {
    const records = this.leaderboard.getRankings();
    this.rankingTableBody.innerHTML = records.map((rec, index) => {
      const rankNum = index + 1;
      const rankClass = `rank-${rankNum}`;
      const medal = rankNum === 1 ? '🥇 ' : rankNum === 2 ? '🥈 ' : rankNum === 3 ? '🥉 ' : `${rankNum}. `;
      return `
        <tr>
          <td class="${rankClass}">${medal}${rankNum}</td>
          <td style="font-weight: bold; color: #ffffff;">${rec.name}</td>
          <td class="neon-yellow" style="font-weight: bold;">${this.formatScore(rec.score)}</td>
          <td style="color: var(--cyan-neon);">${rec.stage}</td>
          <td style="color: #7d96af;">${rec.date}</td>
        </tr>
      `;
    }).join('');
  }

  checkRankingSubmission() {
    if (this.leaderboard.isTopScore(this.score)) {
      this.recordScoreEl.textContent = this.formatScore(this.score);
      this.recordStageEl.textContent = this.state === GAME_STATE.ALL_CLEAR ? 'ALL CLEAR' : `STAGE ${this.currentStageIndex + 1}`;
      this.playerNameInput.value = '';
      this.isModalOpen = true;
      setTimeout(() => {
        this.nameInputModal.classList.remove('hidden');
        this.playerNameInput.focus();
      }, 500);
    }
  }

  clampPaddle() {
    if (this.paddle.x < 0) this.paddle.x = 0;
    if (this.paddle.x + this.paddle.width > CANVAS_WIDTH) {
      this.paddle.x = CANVAS_WIDTH - this.paddle.width;
    }
  }

  handleActionInput() {
    switch (this.state) {
      case GAME_STATE.START:
      case GAME_STATE.BALL_READY:
        this.launchBall();
        break;
      case GAME_STATE.STAGE_CLEAR:
        // ステージクリア待ち時間をスキップして次へ
        this.proceedToNextStage();
        break;
      case GAME_STATE.GAMEOVER:
      case GAME_STATE.ALL_CLEAR:
        this.restartGame();
        break;
      default:
        break;
    }
  }

  // ==========================================
  // 7. アイテムドロップ & パワーアップ
  // ==========================================
  trySpawnItem(x, y) {
    if (Math.random() < 0.30) {
      const types = [
        ITEM_TYPES.MULTI_BALL,
        ITEM_TYPES.EXPAND_PADDLE,
        ITEM_TYPES.PIERCING_BALL,
        ITEM_TYPES.SLOW_BALL
      ];
      const selected = types[Math.floor(Math.random() * types.length)];

      this.fallingItems.push({
        type: selected.id,
        name: selected.name,
        symbol: selected.symbol,
        color: selected.color,
        glow: selected.glow,
        textColor: selected.textColor,
        x: x - 18,
        y: y - 8,
        width: 36,
        height: 16,
        vy: 2.4,
        pulse: 0
      });
    }
  }

  applyPowerup(type) {
    this.sound.itemCollect();
    this.score += 50;
    this.updateUI();

    switch (type) {
      case 'MULTI_BALL': {
        const newBalls = [];
        const sourceBall = this.balls[0] || {
          x: this.paddle.x + this.paddle.width / 2,
          y: this.paddle.y - 12,
          vx: 0,
          vy: -7,
          radius: 7,
          baseSpeed: STAGE_CONFIGS[this.currentStageIndex].baseSpeed
        };

        const currentSpeed = Math.hypot(sourceBall.vx, sourceBall.vy) || sourceBall.baseSpeed;
        const currentAngle = Math.atan2(sourceBall.vy, sourceBall.vx) || -Math.PI / 2;

        [-0.45, 0.45].forEach(deltaAngle => {
          const angle = currentAngle + deltaAngle;
          newBalls.push({
            x: sourceBall.x,
            y: sourceBall.y,
            radius: 7,
            baseSpeed: sourceBall.baseSpeed,
            vx: currentSpeed * Math.cos(angle),
            vy: currentSpeed * Math.sin(angle),
            trail: [],
            piercedSet: new Set()
          });
        });

        this.balls.push(...newBalls);
        this.createItemCollectEffect(this.paddle.x + this.paddle.width / 2, this.paddle.y, ITEM_TYPES.MULTI_BALL.color);
        break;
      }

      case 'EXPAND_PADDLE': {
        this.powerups.expand.active = true;
        this.powerups.expand.timer = 10;
        this.paddle.width = BASE_PADDLE_WIDTH * 1.5;
        this.clampPaddle();
        this.createItemCollectEffect(this.paddle.x + this.paddle.width / 2, this.paddle.y, ITEM_TYPES.EXPAND_PADDLE.color);
        break;
      }

      case 'PIERCING_BALL': {
        this.powerups.pierce.active = true;
        this.powerups.pierce.timer = 8;
        this.createItemCollectEffect(this.paddle.x + this.paddle.width / 2, this.paddle.y, ITEM_TYPES.PIERCING_BALL.color);
        break;
      }

      case 'SLOW_BALL': {
        this.powerups.slow.active = true;
        this.powerups.slow.timer = 10;
        this.createItemCollectEffect(this.paddle.x + this.paddle.width / 2, this.paddle.y, ITEM_TYPES.SLOW_BALL.color);
        break;
      }
    }

    this.updateUI();
  }

  // ==========================================
  // 8. 更新ロジック
  // ==========================================
  update(dt) {
    this.blinkTimer += dt;

    // 背景星
    for (const star of this.stars) {
      star.y += star.speed;
      if (star.y > CANVAS_HEIGHT) {
        star.y = 0;
        star.x = Math.random() * CANVAS_WIDTH;
      }
    }

    if (this.isModalOpen) return;

    // ステージクリア演出待機カウントダウン
    if (this.state === GAME_STATE.STAGE_CLEAR) {
      this.stageClearTimer -= dt;
      if (this.stageClearTimer <= 0) {
        this.proceedToNextStage();
      }
      this.updateParticles();
      return;
    }

    // パドル移動
    if (this.keys.left) this.paddle.x -= this.paddle.speed;
    if (this.keys.right) this.paddle.x += this.paddle.speed;
    this.clampPaddle();

    // パワーアップ減衰
    let powerupChanged = false;
    if (this.powerups.expand.active) {
      this.powerups.expand.timer -= dt;
      if (this.powerups.expand.timer <= 0) {
        this.powerups.expand.active = false;
        this.paddle.width = BASE_PADDLE_WIDTH;
        this.clampPaddle();
        powerupChanged = true;
      }
    }
    if (this.powerups.pierce.active) {
      this.powerups.pierce.timer -= dt;
      if (this.powerups.pierce.timer <= 0) {
        this.powerups.pierce.active = false;
        powerupChanged = true;
      }
    }
    if (this.powerups.slow.active) {
      this.powerups.slow.timer -= dt;
      if (this.powerups.slow.timer <= 0) {
        this.powerups.slow.active = false;
        powerupChanged = true;
      }
    }

    if (this.state === GAME_STATE.START || this.state === GAME_STATE.BALL_READY) {
      if (this.balls.length > 0) {
        this.balls[0].x = this.paddle.x + this.paddle.width / 2;
        this.balls[0].y = this.paddle.y - this.balls[0].radius - 2;
      }
    }

    if (this.state === GAME_STATE.PLAYING) {
      this.updateBalls(dt);
      this.updateFallingItems(dt);
    }

    this.updateParticles();

    if (this.powerups.expand.active || this.powerups.pierce.active || this.powerups.slow.active || powerupChanged) {
      this.updatePowerupBanner();
    }
  }

  updateFallingItems(dt) {
    for (let i = this.fallingItems.length - 1; i >= 0; i--) {
      const item = this.fallingItems[i];
      item.y += item.vy;
      item.pulse += dt * 5;

      if (
        item.y + item.height >= this.paddle.y &&
        item.y <= this.paddle.y + this.paddle.height &&
        item.x + item.width >= this.paddle.x &&
        item.x <= this.paddle.x + this.paddle.width
      ) {
        this.applyPowerup(item.type);
        this.fallingItems.splice(i, 1);
        continue;
      }

      if (item.y > CANVAS_HEIGHT) {
        this.fallingItems.splice(i, 1);
      }
    }
  }

  updateBalls(dt) {
    const isSlow = this.powerups.slow.active;
    const speedMult = isSlow ? 0.7 : 1.0;
    const isPiercing = this.powerups.pierce.active;

    for (let i = this.balls.length - 1; i >= 0; i--) {
      const ball = this.balls[i];

      ball.trail.push({ x: ball.x, y: ball.y });
      if (ball.trail.length > 8) ball.trail.shift();

      ball.x += ball.vx * speedMult;
      ball.y += ball.vy * speedMult;

      // 左右壁
      if (ball.x - ball.radius < 0) {
        ball.x = ball.radius;
        ball.vx = Math.abs(ball.vx);
        this.sound.wallHit();
        this.createSpark(ball.x, ball.y, '#00f0ff', 6);
      } else if (ball.x + ball.radius > CANVAS_WIDTH) {
        ball.x = CANVAS_WIDTH - ball.radius;
        ball.vx = -Math.abs(ball.vx);
        this.sound.wallHit();
        this.createSpark(ball.x, ball.y, '#00f0ff', 6);
      }

      // 天井
      if (ball.y - ball.radius < 0) {
        ball.y = ball.radius;
        ball.vy = Math.abs(ball.vy);
        this.sound.wallHit();
        this.createSpark(ball.x, ball.y, '#00f0ff', 6);
      }

      // パドル
      if (
        ball.y + ball.radius >= this.paddle.y &&
        ball.y - ball.radius <= this.paddle.y + this.paddle.height &&
        ball.x + ball.radius >= this.paddle.x &&
        ball.x - ball.radius <= this.paddle.x + this.paddle.width
      ) {
        if (ball.vy > 0) {
          ball.y = this.paddle.y - ball.radius;

          const paddleCenter = this.paddle.x + this.paddle.width / 2;
          const hitOffset = (ball.x - paddleCenter) / (this.paddle.width / 2);
          const clampedOffset = Math.max(-0.92, Math.min(0.92, hitOffset));

          const maxAngle = (60 * Math.PI) / 180;
          const reflectionAngle = clampedOffset * maxAngle;

          const currentSpeed = Math.min(13, Math.hypot(ball.vx, ball.vy) * 1.02);

          ball.vx = currentSpeed * Math.sin(reflectionAngle);
          ball.vy = -currentSpeed * Math.cos(reflectionAngle);

          ball.piercedSet.clear();
          this.sound.paddleHit();
          this.combo = 0;
          this.createSpark(ball.x, ball.y, '#00f0ff', 10);
        }
      }

      // ブロック衝突
      this.checkBrickCollisionForBall(ball, isPiercing);

      // 下端落下
      if (ball.y - ball.radius > CANVAS_HEIGHT) {
        this.balls.splice(i, 1);
        this.createExplosion(ball.x, CANVAS_HEIGHT - 10, '#ff007f', 15);
      }
    }

    // 全ボール落下時のミス判定
    if (this.balls.length === 0) {
      this.lives--;
      this.sound.lifeLost();
      this.clearPowerups();
      this.updateUI();

      if (this.lives <= 0) {
        this.state = GAME_STATE.GAMEOVER;
        this.sound.gameOver();
        this.checkRankingSubmission();
      } else {
        this.state = GAME_STATE.BALL_READY;
        this.initBallOnPaddle();
      }
    }
  }

  checkBrickCollisionForBall(ball, isPiercing) {
    const rows = this.bricks.length;
    for (let r = 0; r < rows; r++) {
      const cols = this.bricks[r].length;
      for (let c = 0; c < cols; c++) {
        const b = this.bricks[r][c];
        if (!b || !b.alive) continue;

        // 貫通中ボールは同一ブロックとの多重判定を回避
        if (isPiercing && ball.piercedSet.has(b.id)) {
          continue;
        }

        const closestX = Math.max(b.x, Math.min(ball.x, b.x + b.width));
        const closestY = Math.max(b.y, Math.min(ball.y, b.y + b.height));

        const distX = ball.x - closestX;
        const distY = ball.y - closestY;
        const distSquared = distX * distX + distY * distY;

        if (distSquared < ball.radius * ball.radius) {
          // メタルブロック（破壊不能）の衝突処理
          if (b.indestructible) {
            this.sound.metalHit();
            this.createMetalSparks(closestX, closestY);

            // メタルブロックは貫通時も含めて常に跳ね返す
            this.reflectBallFromAABB(ball, b);
            return;
          }

          // 通常・硬質ブロックの衝突処理
          this.combo++;
          b.hp--;

          // 耐久ブロック叩きスコア（ヒットボーナス + コンボ）
          const hitScore = 15 + (this.combo - 1) * 5;
          this.score += hitScore;

          if (isPiercing) {
            // 貫通時は跳ね返らず直進
            ball.piercedSet.add(b.id);
            this.createPierceSparks(closestX, closestY);
          } else {
            this.reflectBallFromAABB(ball, b);
          }

          // 耐久値ゼロで破壊
          if (b.hp <= 0) {
            b.alive = false;
            this.sound.brickBreak();
            this.score += b.points; // 破壊ボーナス
            this.createBrickDebris(b.x, b.y, b.width, b.height, b.glow);
            this.trySpawnItem(b.x + b.width / 2, b.y + b.height / 2);
          } else {
            // 硬質ブロックダメージ時（色を更新）
            const newColor = this.getBrickColorByHp(b.hp);
            b.color = newColor.color;
            b.glow = newColor.glow;
            this.sound.brickHit();
            this.createSpark(closestX, closestY, b.glow, 8);
          }

          this.updateUI();

          if (!isPiercing) return;
        }
      }
    }

    // ステージクリア判定（破壊可能ブロックがゼロになったか）
    let remainingDestructible = 0;
    for (let r = 0; r < rows; r++) {
      for (let c = 0; c < this.bricks[r].length; c++) {
        const b = this.bricks[r][c];
        if (b && b.alive && !b.indestructible) {
          remainingDestructible++;
        }
      }
    }

    if (remainingDestructible === 0 && this.state === GAME_STATE.PLAYING) {
      this.handleStageCompleted();
    }
  }

  reflectBallFromAABB(ball, b) {
    const overlapLeft = (ball.x + ball.radius) - b.x;
    const overlapRight = (b.x + b.width) - (ball.x - ball.radius);
    const overlapTop = (ball.y + ball.radius) - b.y;
    const overlapBottom = (b.y + b.height) - (ball.y - ball.radius);

    const minOverlapX = Math.min(overlapLeft, overlapRight);
    const minOverlapY = Math.min(overlapTop, overlapBottom);

    if (minOverlapX < minOverlapY) {
      ball.vx = -ball.vx;
    } else {
      ball.vy = -ball.vy;
    }
  }

  handleStageCompleted() {
    // ステージクリアボーナス計算: 残機 × 500点
    this.stageClearBonus = this.lives * 500;
    this.score += this.stageClearBonus;
    this.updateUI();

    if (this.currentStageIndex < STAGE_CONFIGS.length - 1) {
      // 次のステージへ遷移待機
      this.state = GAME_STATE.STAGE_CLEAR;
      this.stageClearTimer = 3.5; // 3.5秒後に自動開始
      this.sound.stageClear();
      this.createExplosion(CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2, '#00f0ff', 40);
    } else {
      // 全ステージクリア！
      this.state = GAME_STATE.ALL_CLEAR;
      const allClearBonus = 3000;
      this.score += allClearBonus;
      this.updateUI();
      this.sound.victory();
      this.createExplosion(CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2, '#ffe600', 70);
      this.checkRankingSubmission();
    }
  }

  proceedToNextStage() {
    this.currentStageIndex++;
    this.clearPowerups();
    this.loadStage(this.currentStageIndex);
    this.initBallOnPaddle();
    this.updateUI();
    this.state = GAME_STATE.BALL_READY;
  }

  // ==========================================
  // 9. パーティクル演出
  // ==========================================
  createSpark(x, y, color, count = 6) {
    for (let i = 0; i < count; i++) {
      const angle = Math.random() * Math.PI * 2;
      const speed = Math.random() * 4 + 1;
      this.particles.push({
        x, y,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed,
        size: Math.random() * 3 + 2,
        color,
        alpha: 1.0,
        decay: Math.random() * 0.04 + 0.02,
        type: 'dot'
      });
    }
  }

  createMetalSparks(x, y) {
    for (let i = 0; i < 10; i++) {
      const angle = Math.random() * Math.PI * 2;
      const speed = Math.random() * 5 + 2;
      this.particles.push({
        x, y,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed,
        size: Math.random() * 3 + 1.5,
        color: '#ffffff',
        alpha: 1.0,
        decay: 0.05,
        type: 'dot'
      });
    }
  }

  createBrickDebris(bx, by, bw, bh, color) {
    for (let i = 0; i < 12; i++) {
      const angle = Math.random() * Math.PI * 2;
      const speed = Math.random() * 5 + 2;
      this.particles.push({
        x: bx + Math.random() * bw,
        y: by + Math.random() * bh,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed - 1.5,
        size: Math.random() * 5 + 3,
        color,
        alpha: 1.0,
        decay: Math.random() * 0.03 + 0.015,
        rotation: Math.random() * Math.PI * 2,
        vRot: (Math.random() - 0.5) * 0.2,
        type: 'debris'
      });
    }
    this.createSpark(bx + bw / 2, by + bh / 2, color, 10);
  }

  createPierceSparks(x, y) {
    for (let i = 0; i < 8; i++) {
      const angle = Math.random() * Math.PI * 2;
      const speed = Math.random() * 5 + 2;
      this.particles.push({
        x, y,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed,
        size: 3,
        color: '#ff2255',
        alpha: 1.0,
        decay: 0.06,
        type: 'dot'
      });
    }
  }

  createItemCollectEffect(x, y, color) {
    for (let i = 0; i < 18; i++) {
      const angle = Math.random() * Math.PI * 2;
      const speed = Math.random() * 5 + 2;
      this.particles.push({
        x, y,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed - 1.0,
        size: Math.random() * 4 + 2,
        color,
        alpha: 1.0,
        decay: 0.03,
        type: 'ring'
      });
    }
  }

  createExplosion(x, y, color, count = 20) {
    for (let i = 0; i < count; i++) {
      const angle = Math.random() * Math.PI * 2;
      const speed = Math.random() * 6 + 1.5;
      this.particles.push({
        x, y,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed,
        size: Math.random() * 4 + 2,
        color,
        alpha: 1.0,
        decay: Math.random() * 0.025 + 0.015,
        type: 'dot'
      });
    }
  }

  updateParticles() {
    for (let i = this.particles.length - 1; i >= 0; i--) {
      const p = this.particles[i];
      p.x += p.vx;
      p.y += p.vy;
      p.vx *= 0.96;
      p.vy *= 0.96;
      if (p.rotation !== undefined) p.rotation += p.vRot;
      p.alpha -= p.decay;

      if (p.alpha <= 0) {
        this.particles.splice(i, 1);
      }
    }
  }

  // ==========================================
  // 10. 描画ロジック
  // ==========================================
  draw() {
    const ctx = this.ctx;
    ctx.clearRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);

    this.drawStars(ctx);
    this.drawFloorGrid(ctx);
    this.drawBricks(ctx);
    this.drawFallingItems(ctx);
    this.drawPaddle(ctx);
    this.drawBalls(ctx);
    this.drawParticles(ctx);
    this.drawCanvasPowerupHUD(ctx);
    this.drawOverlay(ctx);
  }

  drawStars(ctx) {
    ctx.save();
    for (const star of this.stars) {
      ctx.fillStyle = `rgba(0, 240, 255, ${star.alpha})`;
      ctx.fillRect(star.x, star.y, star.size, star.size);
    }
    ctx.restore();
  }

  drawFloorGrid(ctx) {
    ctx.save();
    ctx.strokeStyle = 'rgba(0, 240, 255, 0.08)';
    ctx.lineWidth = 1;

    const horizon = CANVAS_HEIGHT - 80;
    for (let y = horizon; y <= CANVAS_HEIGHT; y += 15) {
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(CANVAS_WIDTH, y);
      ctx.stroke();
    }
    for (let x = -100; x <= CANVAS_WIDTH + 100; x += 50) {
      ctx.beginPath();
      ctx.moveTo(x, horizon);
      ctx.lineTo(CANVAS_WIDTH / 2 + (x - CANVAS_WIDTH / 2) * 2.5, CANVAS_HEIGHT);
      ctx.stroke();
    }
    ctx.restore();
  }

  drawPaddle(ctx) {
    const p = this.paddle;
    ctx.save();

    const isExpand = this.powerups.expand.active;
    const paddleColor = isExpand ? '#00ff88' : p.color;

    ctx.shadowColor = paddleColor;
    ctx.shadowBlur = isExpand ? 22 : 15;

    const grad = ctx.createLinearGradient(p.x, p.y, p.x, p.y + p.height);
    grad.addColorStop(0, '#ffffff');
    grad.addColorStop(0.3, paddleColor);
    grad.addColorStop(1, isExpand ? '#00552b' : '#0077aa');

    ctx.fillStyle = grad;
    this.drawRoundedRect(ctx, p.x, p.y, p.width, p.height, 6);
    ctx.fill();

    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 1.5;
    ctx.shadowBlur = 4;
    ctx.beginPath();
    ctx.moveTo(p.x + 8, p.y + 3);
    ctx.lineTo(p.x + p.width - 8, p.y + 3);
    ctx.stroke();

    ctx.restore();
  }

  drawBalls(ctx) {
    const isPiercing = this.powerups.pierce.active;

    for (const b of this.balls) {
      ctx.save();

      const trailColor = isPiercing ? 'rgba(255, 34, 85,' : 'rgba(255, 0, 127,';
      for (let i = 0; i < b.trail.length; i++) {
        const pos = b.trail[i];
        const ratio = (i + 1) / b.trail.length;
        ctx.beginPath();
        ctx.arc(pos.x, pos.y, b.radius * ratio * 0.9, 0, Math.PI * 2);
        ctx.fillStyle = `${trailColor} ${ratio * 0.45})`;
        ctx.fill();
      }

      const ballGlow = isPiercing ? '#ff2255' : '#ff007f';
      ctx.shadowColor = ballGlow;
      ctx.shadowBlur = isPiercing ? 24 : 16;

      const ballGrad = ctx.createRadialGradient(
        b.x - 2, b.y - 2, 1,
        b.x, b.y, b.radius
      );
      if (isPiercing) {
        ballGrad.addColorStop(0, '#ffffff');
        ballGrad.addColorStop(0.4, '#ff6688');
        ballGrad.addColorStop(1, '#ff0033');
      } else {
        ballGrad.addColorStop(0, '#ffffff');
        ballGrad.addColorStop(0.5, '#ff80bf');
        ballGrad.addColorStop(1, '#ff007f');
      }

      ctx.fillStyle = ballGrad;
      ctx.beginPath();
      ctx.arc(b.x, b.y, b.radius, 0, Math.PI * 2);
      ctx.fill();

      ctx.restore();
    }
  }

  drawBricks(ctx) {
    ctx.save();
    for (let r = 0; r < this.bricks.length; r++) {
      for (let c = 0; c < this.bricks[r].length; c++) {
        const b = this.bricks[r][c];
        if (!b || !b.alive) continue;

        if (b.indestructible) {
          // メタルブロック（シルバーメタル & リベット）
          ctx.shadowColor = '#ffffff';
          ctx.shadowBlur = 6;

          const metalGrad = ctx.createLinearGradient(b.x, b.y, b.x, b.y + b.height);
          metalGrad.addColorStop(0, '#e8ecf4');
          metalGrad.addColorStop(0.5, '#78849b');
          metalGrad.addColorStop(1, '#3a4456');

          ctx.fillStyle = metalGrad;
          this.drawRoundedRect(ctx, b.x, b.y, b.width, b.height, 4);
          ctx.fill();

          ctx.strokeStyle = '#d0d8e8';
          ctx.lineWidth = 1.5;
          ctx.stroke();

          // メタルのリベット鋲（四隅）
          ctx.fillStyle = '#1e2430';
          const rivetOffset = 4;
          [
            [b.x + rivetOffset, b.y + rivetOffset],
            [b.x + b.width - rivetOffset, b.y + rivetOffset],
            [b.x + rivetOffset, b.y + b.height - rivetOffset],
            [b.x + b.width - rivetOffset, b.y + b.height - rivetOffset]
          ].forEach(([rx, ry]) => {
            ctx.beginPath();
            ctx.arc(rx, ry, 1.5, 0, Math.PI * 2);
            ctx.fill();
          });
        } else {
          // 通常・硬質ブロック
          const hpRatio = b.hp / b.maxHp;
          ctx.shadowColor = b.glow;
          ctx.shadowBlur = 8 + (hpRatio * 6);

          const grad = ctx.createLinearGradient(b.x, b.y, b.x, b.y + b.height);
          grad.addColorStop(0, '#ffffff');
          grad.addColorStop(0.2, b.color);
          grad.addColorStop(1, '#08081a');

          ctx.fillStyle = grad;
          this.drawRoundedRect(ctx, b.x, b.y, b.width, b.height, 4);
          ctx.fill();

          ctx.strokeStyle = b.glow;
          ctx.lineWidth = 1.2;
          ctx.stroke();

          // 耐久残数インジケータ（ドット）
          if (b.hp > 1) {
            ctx.fillStyle = '#ffffff';
            for (let i = 0; i < b.hp; i++) {
              ctx.beginPath();
              ctx.arc(b.x + b.width / 2 + (i - (b.hp - 1) / 2) * 8, b.y + b.height / 2, 2.2, 0, Math.PI * 2);
              ctx.fill();
            }
          }
        }
      }
    }
    ctx.restore();
  }

  drawFallingItems(ctx) {
    for (const item of this.fallingItems) {
      ctx.save();
      ctx.shadowColor = item.glow;
      ctx.shadowBlur = 12;

      const grad = ctx.createLinearGradient(item.x, item.y, item.x, item.y + item.height);
      grad.addColorStop(0, '#ffffff');
      grad.addColorStop(0.4, item.color);
      grad.addColorStop(1, '#05040a');

      ctx.fillStyle = grad;
      this.drawRoundedRect(ctx, item.x, item.y, item.width, item.height, 8);
      ctx.fill();

      ctx.strokeStyle = item.color;
      ctx.lineWidth = 1.5;
      ctx.stroke();

      ctx.font = 'bold 10px "Consolas", monospace';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillStyle = item.textColor;
      ctx.shadowBlur = 0;
      ctx.fillText(item.symbol, item.x + item.width / 2, item.y + item.height / 2);

      ctx.restore();
    }
  }

  drawCanvasPowerupHUD(ctx) {
    const activeList = [];
    if (this.powerups.expand.active) {
      activeList.push({
        label: 'EXPAND PADDLE',
        color: ITEM_TYPES.EXPAND_PADDLE.color,
        timer: this.powerups.expand.timer,
        max: this.powerups.expand.maxDuration
      });
    }
    if (this.powerups.pierce.active) {
      activeList.push({
        label: 'PIERCING BALL',
        color: ITEM_TYPES.PIERCING_BALL.color,
        timer: this.powerups.pierce.timer,
        max: this.powerups.pierce.maxDuration
      });
    }
    if (this.powerups.slow.active) {
      activeList.push({
        label: 'SLOW MOTION',
        color: ITEM_TYPES.SLOW_BALL.color,
        timer: this.powerups.slow.timer,
        max: this.powerups.slow.maxDuration
      });
    }

    if (activeList.length === 0) return;

    ctx.save();
    const itemWidth = 140;
    const totalWidth = activeList.length * itemWidth + (activeList.length - 1) * 12;
    let startX = (CANVAS_WIDTH - totalWidth) / 2;
    const startY = 16;

    for (const p of activeList) {
      const isUrgent = p.timer < 2.0;
      const blink = isUrgent && Math.sin(this.blinkTimer * 12) > 0;

      ctx.shadowColor = p.color;
      ctx.shadowBlur = isUrgent ? 15 : 8;

      ctx.fillStyle = blink ? 'rgba(40, 10, 20, 0.85)' : 'rgba(10, 8, 24, 0.85)';
      this.drawRoundedRect(ctx, startX, startY, itemWidth, 24, 6);
      ctx.fill();

      ctx.strokeStyle = p.color;
      ctx.lineWidth = 1.2;
      ctx.stroke();

      ctx.font = 'bold 9px "Consolas", monospace';
      ctx.fillStyle = p.color;
      ctx.textAlign = 'left';
      ctx.textBaseline = 'top';
      ctx.fillText(`${p.label} ${p.timer.toFixed(1)}s`, startX + 8, startY + 4);

      const gaugeWidth = itemWidth - 16;
      const gaugeHeight = 4;
      const gaugeY = startY + 16;
      const pct = Math.max(0, Math.min(1, p.timer / p.max));

      ctx.fillStyle = 'rgba(255, 255, 255, 0.15)';
      ctx.fillRect(startX + 8, gaugeY, gaugeWidth, gaugeHeight);

      ctx.fillStyle = p.color;
      ctx.fillRect(startX + 8, gaugeY, gaugeWidth * pct, gaugeHeight);

      startX += itemWidth + 12;
    }

    ctx.restore();
  }

  drawParticles(ctx) {
    ctx.save();
    for (const p of this.particles) {
      ctx.globalAlpha = Math.max(0, p.alpha);
      ctx.shadowColor = p.color;
      ctx.shadowBlur = 8;
      ctx.fillStyle = p.color;

      if (p.type === 'debris') {
        ctx.save();
        ctx.translate(p.x, p.y);
        ctx.rotate(p.rotation || 0);
        ctx.fillRect(-p.size / 2, -p.size / 2, p.size, p.size);
        ctx.restore();
      } else if (p.type === 'ring') {
        ctx.strokeStyle = p.color;
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        ctx.arc(p.x, p.y, p.size * 2, 0, Math.PI * 2);
        ctx.stroke();
      } else {
        ctx.beginPath();
        ctx.arc(p.x, p.y, p.size, 0, Math.PI * 2);
        ctx.fill();
      }
    }
    ctx.restore();
  }

  drawOverlay(ctx) {
    ctx.save();
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';

    const blink = Math.sin(this.blinkTimer * 5) > 0;

    if (this.state === GAME_STATE.START) {
      this.drawModalBackdrop(ctx);
      ctx.font = 'bold 36px "Consolas", monospace';
      ctx.fillStyle = '#00f0ff';
      ctx.shadowColor = '#00f0ff';
      ctx.shadowBlur = 15;
      ctx.fillText(STAGE_CONFIGS[this.currentStageIndex].name, CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2 - 35);

      if (blink) {
        ctx.font = 'bold 20px "Consolas", monospace';
        ctx.fillStyle = '#ffe600';
        ctx.shadowColor = '#ffe600';
        ctx.shadowBlur = 10;
        ctx.fillText('PRESS SPACE OR CLICK TO LAUNCH', CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2 + 25);
      }
    } else if (this.state === GAME_STATE.BALL_READY) {
      if (blink) {
        ctx.font = 'bold 20px "Consolas", monospace';
        ctx.fillStyle = '#ff007f';
        ctx.shadowColor = '#ff007f';
        ctx.shadowBlur = 12;
        ctx.fillText('PRESS SPACE OR CLICK TO LAUNCH', CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2 + 40);
      }
    } else if (this.state === GAME_STATE.STAGE_CLEAR) {
      this.drawModalBackdrop(ctx);
      ctx.font = 'bold 42px "Consolas", monospace';
      ctx.fillStyle = '#00ff88';
      ctx.shadowColor = '#00ff88';
      ctx.shadowBlur = 20;
      ctx.fillText(`STAGE ${this.currentStageIndex + 1} CLEAR!`, CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2 - 45);

      ctx.font = '20px "Consolas", monospace';
      ctx.fillStyle = '#ffe600';
      ctx.shadowBlur = 8;
      ctx.fillText(`LIFE BONUS: +${this.formatScore(this.stageClearBonus)} PTS`, CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2 + 5);

      const secLeft = Math.ceil(this.stageClearTimer);
      ctx.font = '16px "Consolas", monospace';
      ctx.fillStyle = '#e0f8ff';
      ctx.fillText(`NEXT STAGE IN ${secLeft}s (OR PRESS SPACE / CLICK)`, CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2 + 50);
    } else if (this.state === GAME_STATE.GAMEOVER) {
      this.drawModalBackdrop(ctx);
      ctx.font = 'bold 44px "Consolas", monospace';
      ctx.fillStyle = '#ff007f';
      ctx.shadowColor = '#ff007f';
      ctx.shadowBlur = 20;
      ctx.fillText('GAME OVER', CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2 - 40);

      ctx.font = '20px "Consolas", monospace';
      ctx.fillStyle = '#e0f8ff';
      ctx.shadowBlur = 5;
      ctx.fillText(`FINAL SCORE: ${this.formatScore(this.score)}`, CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2 + 10);

      if (blink) {
        ctx.font = 'bold 18px "Consolas", monospace';
        ctx.fillStyle = '#ffe600';
        ctx.shadowColor = '#ffe600';
        ctx.shadowBlur = 10;
        ctx.fillText('PRESS SPACE OR CLICK TO RETRY', CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2 + 65);
      }
    } else if (this.state === GAME_STATE.ALL_CLEAR) {
      this.drawModalBackdrop(ctx);
      ctx.font = 'bold 44px "Consolas", monospace';
      ctx.fillStyle = '#ffe600';
      ctx.shadowColor = '#ffe600';
      ctx.shadowBlur = 25;
      ctx.fillText('ALL STAGES CLEARED!', CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2 - 45);

      ctx.font = '22px "Consolas", monospace';
      ctx.fillStyle = '#00ff88';
      ctx.shadowColor = '#00ff88';
      ctx.shadowBlur = 10;
      ctx.fillText(`CONGRATULATIONS! SCORE: ${this.formatScore(this.score)}`, CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2 + 5);

      if (blink) {
        ctx.font = 'bold 18px "Consolas", monospace';
        ctx.fillStyle = '#00f0ff';
        ctx.shadowColor = '#00f0ff';
        ctx.shadowBlur = 10;
        ctx.fillText('PRESS SPACE OR CLICK TO PLAY AGAIN', CANVAS_WIDTH / 2, CANVAS_HEIGHT / 2 + 65);
      }
    }

    ctx.restore();
  }

  drawModalBackdrop(ctx) {
    ctx.fillStyle = 'rgba(5, 3, 15, 0.75)';
    ctx.fillRect(CANVAS_WIDTH / 2 - 280, CANVAS_HEIGHT / 2 - 105, 560, 210);

    ctx.strokeStyle = 'rgba(0, 240, 255, 0.4)';
    ctx.lineWidth = 2;
    ctx.strokeRect(CANVAS_WIDTH / 2 - 280, CANVAS_HEIGHT / 2 - 105, 560, 210);
  }

  drawRoundedRect(ctx, x, y, width, height, radius) {
    ctx.beginPath();
    ctx.moveTo(x + radius, y);
    ctx.lineTo(x + width - radius, y);
    ctx.quadraticCurveTo(x + width, y, x + width, y + radius);
    ctx.lineTo(x + width, y + height - radius);
    ctx.quadraticCurveTo(x + width, y + height, x + width - radius, y + height);
    ctx.lineTo(x + radius, y + height);
    ctx.quadraticCurveTo(x, y + height, x, y + height - radius);
    ctx.lineTo(x, y + radius);
    ctx.quadraticCurveTo(x, y, x + radius, y);
    ctx.closePath();
  }

  // ==========================================
  // 11. メインゲームループ
  // ==========================================
  gameLoop(timestamp) {
    if (!this.lastTime) this.lastTime = timestamp;
    const dt = Math.min((timestamp - this.lastTime) / 1000, 0.1);
    this.lastTime = timestamp;

    this.update(dt);
    this.draw();

    requestAnimationFrame((ts) => this.gameLoop(ts));
  }
}

window.addEventListener('DOMContentLoaded', () => {
  new NeonBreakoutGame();
});
