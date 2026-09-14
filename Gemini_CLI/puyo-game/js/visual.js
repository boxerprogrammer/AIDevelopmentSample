/**
 * js/visual.js
 * ぷよの表情・連結グラフィック描画、パーティクル爆発エフェクト、画面揺れ、カットイン
 */

// ぷよのカラーパレット定義
const PUYO_COLORS = {
  1: { // 赤 (Red)
    base: '#ff3366',
    light: '#ff7597',
    shadow: '#b8123c',
    glow: 'rgba(255, 51, 102, 0.6)',
    eyeType: 'cute'
  },
  2: { // 青 (Blue)
    base: '#2979ff',
    light: '#75a7ff',
    shadow: '#0d47a1',
    glow: 'rgba(41, 121, 255, 0.6)',
    eyeType: 'droopy'
  },
  3: { // 緑 (Green)
    base: '#00e676',
    light: '#69f0ae',
    shadow: '#008e3e',
    glow: 'rgba(0, 230, 118, 0.6)',
    eyeType: 'gentle'
  },
  4: { // 黄 (Yellow)
    base: '#ffd600',
    light: '#ffff52',
    shadow: '#c79100',
    glow: 'rgba(255, 214, 0, 0.6)',
    eyeType: 'wide'
  },
  5: { // 紫 (Purple)
    base: '#d500f9',
    light: '#e040fb',
    shadow: '#7b00ab',
    glow: 'rgba(213, 0, 249, 0.6)',
    eyeType: 'sharp'
  },
  9: { // おじゃまぷよ (Garbage/Ojama)
    base: '#cfd8dc',
    light: '#ffffff',
    shadow: '#78909c',
    glow: 'rgba(207, 216, 220, 0.6)',
    eyeType: 'ojama'
  }
};

class ParticleSystem {
  constructor() {
    this.particles = [];
  }

  /**
   * ぷよ消滅時の放射状スパークパーティクル
   * @param {number} x 中心座標X
   * @param {number} y 中心座標Y
   * @param {number} puyoType ぷよ種別
   * @param {number} count 粒子数
   */
  explode(x, y, puyoType, count = 28) {
    const palette = PUYO_COLORS[puyoType] || PUYO_COLORS[9];
    
    for (let i = 0; i < count; i++) {
      const angle = Math.random() * Math.PI * 2;
      const speed = Math.random() * 6 + 2;
      const size = Math.random() * 6 + 3;
      const life = Math.random() * 20 + 25; // 25〜45フレーム

      this.particles.push({
        x: x + (Math.random() - 0.5) * 12,
        y: y + (Math.random() - 0.5) * 12,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed - 1.5,
        color: (Math.random() > 0.3) ? palette.base : palette.light,
        size: size,
        alpha: 1.0,
        life: life,
        maxLife: life,
        gravity: 0.18,
        glow: palette.glow
      });
    }

    // キラキラ星パーティクル追加
    for (let i = 0; i < 6; i++) {
      const angle = Math.random() * Math.PI * 2;
      const speed = Math.random() * 4 + 1;
      this.particles.push({
        x, y,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed - 2,
        color: '#ffffff',
        size: 3.5,
        alpha: 1.0,
        life: 30,
        maxLife: 30,
        gravity: 0.08,
        isStar: true
      });
    }
  }

  update() {
    for (let i = this.particles.length - 1; i >= 0; i--) {
      const p = this.particles[i];
      p.x += p.vx;
      p.y += p.vy;
      p.vy += p.gravity;
      p.vx *= 0.96;
      p.life--;
      p.alpha = Math.max(0, p.life / p.maxLife);

      if (p.life <= 0) {
        this.particles.splice(i, 1);
      }
    }
  }

  draw(ctx) {
    ctx.save();
    for (const p of this.particles) {
      ctx.globalAlpha = p.alpha;
      ctx.fillStyle = p.color;

      if (p.isStar) {
        // 十字のキラキラ描画
        ctx.beginPath();
        ctx.arc(p.x, p.y, p.size * 0.7, 0, Math.PI * 2);
        ctx.fill();
      } else {
        ctx.beginPath();
        ctx.arc(p.x, p.y, p.size * (p.alpha * 0.7 + 0.3), 0, Math.PI * 2);
        ctx.fill();
      }
    }
    ctx.restore();
  }

  clear() {
    this.particles = [];
  }
}

class VisualRenderer {
  constructor() {
    this.cellSize = 48; // 盤面マスサイズ (288 / 6 = 48)
    this.blinkTimer = 0;
    this.shakeAmount = 0;
  }

  /** グローバルアニメーション更新 (まばたき・振動) */
  updateTime(tick) {
    this.blinkTimer = tick % 180; // 約3秒周期
  }

  /** 画面揺れトリガー */
  triggerShake(intensity = 8) {
    this.shakeAmount = Math.max(this.shakeAmount, intensity);
  }

  /**
   * 単体または連結ぷよの描画
   * @param {CanvasRenderingContext2D} ctx 描画コンテキスト
   * @param {number} cx 描画中心X
   * @param {number} cy 描画中心Y
   * @param {number} type ぷよ種別 (1〜5, 9)
   * @param {Object} connections { up, down, left, right }
   * @param {Object} state { shaking, flash, scaleX, scaleY, offsetAnim }
   */
  drawPuyo(ctx, cx, cy, type, connections = {}, state = {}) {
    if (!type || type <= 0) return;

    const palette = PUYO_COLORS[type] || PUYO_COLORS[9];
    const size = this.cellSize;
    const r = size * 0.43; // 半径約20.6px

    ctx.save();
    ctx.translate(cx, cy);

    // ぷるぷる振動 (消滅前演出)
    if (state.shaking) {
      const shakeX = (Math.random() - 0.5) * 5;
      const shakeY = (Math.random() - 0.5) * 5;
      ctx.translate(shakeX, shakeY);
    }

    // スケール変形 (着地バウンスや落下変形)
    const sx = state.scaleX || 1.0;
    const sy = state.scaleY || 1.0;
    ctx.scale(sx, sy);

    // 1. ボディのパス生成 (滑らかな連結形状)
    ctx.beginPath();

    // 各方向の接続判定
    const { up, down, left, right } = connections;

    // 基本円 + 接続ブリッジ
    this.buildConnectedPuyoPath(ctx, r, size, up, down, left, right);

    // 影付き外枠ストローク (ダークボーダー)
    ctx.lineWidth = 4;
    ctx.strokeStyle = palette.shadow;
    ctx.stroke();

    // 2. 本体塗りつぶし (立体感グラデーション)
    const bodyGrad = ctx.createRadialGradient(-r * 0.3, -r * 0.3, r * 0.1, 0, 0, r * 1.3);
    bodyGrad.addColorStop(0, palette.light);
    bodyGrad.addColorStop(0.6, palette.base);
    bodyGrad.addColorStop(1, palette.shadow);

    ctx.fillStyle = bodyGrad;
    ctx.fill();

    // 3. 発光フラッシュ (4個以上揃った時の発光)
    if (state.flash) {
      ctx.fillStyle = 'rgba(255, 255, 255, 0.6)';
      ctx.fill();
    }

    // 4. ゼリーハイライト (上部の光沢反射光)
    this.drawHighlight(ctx, r, up, left);

    // 5. 目・表情描画
    this.drawEyes(ctx, palette.eyeType, state);

    ctx.restore();
  }

  /**
   * 連結ぷよのパス生成
   * 上下左右の隣接ぷよとくびれなく結合するベジェ・ラウンド矩形パス
   */
  buildConnectedPuyoPath(ctx, r, size, up, down, left, right) {
    const half = size / 2;
    const bridgeW = r * 0.95; // 接続部の幅

    // 接続がない場合はシンプルなぷよ丸型 (少し下垂した自然な雫シェイプ)
    if (!up && !down && !left && !right) {
      ctx.arc(0, 0, r, 0, Math.PI * 2);
      return;
    }

    // 接続がある場合: 中心円に加え、各方向へブリッジを張り巡らせる
    ctx.arc(0, 0, r, 0, Math.PI * 2);

    if (up) {
      ctx.rect(-bridgeW, -half, bridgeW * 2, half);
    }
    if (down) {
      ctx.rect(-bridgeW, 0, bridgeW * 2, half);
    }
    if (left) {
      ctx.rect(-half, -bridgeW, half, bridgeW * 2);
    }
    if (right) {
      ctx.rect(0, -bridgeW, half, bridgeW * 2);
    }
  }

  /** 光沢ハイライト */
  drawHighlight(ctx, r, up, left) {
    ctx.save();
    ctx.beginPath();
    // ぷよの左上に三日月状の白い反射光
    const hx = left ? -r * 0.3 : -r * 0.45;
    const hy = up ? -r * 0.3 : -r * 0.45;
    ctx.ellipse(hx, hy, r * 0.35, r * 0.22, -Math.PI / 4, 0, Math.PI * 2);
    ctx.fillStyle = 'rgba(255, 255, 255, 0.65)';
    ctx.fill();

    // 右下の小さな光彩
    ctx.beginPath();
    ctx.arc(r * 0.35, r * 0.35, r * 0.12, 0, Math.PI * 2);
    ctx.fillStyle = 'rgba(255, 255, 255, 0.3)';
    ctx.fill();
    ctx.restore();
  }

  /** 表情・目の描画 */
  drawEyes(ctx, eyeType, state) {
    ctx.save();

    // まばたき判定 (180フレーム中、5フレームだけ閉じる)
    const isBlinking = (this.blinkTimer > 175);
    const isClearing = state.shaking || state.flash;

    if (eyeType === 'ojama') {
      // おじゃまぷよ: 中央の不気味かつコミカルな赤い目
      ctx.fillStyle = '#ffffff';
      ctx.beginPath();
      ctx.arc(0, 0, 9, 0, Math.PI * 2);
      ctx.fill();
      ctx.lineWidth = 1.5;
      ctx.strokeStyle = '#37474f';
      ctx.stroke();

      // 赤い瞳
      ctx.fillStyle = '#ff1744';
      ctx.beginPath();
      ctx.arc(0, 0, 5, 0, Math.PI * 2);
      ctx.fill();

      // ハイライト
      ctx.fillStyle = '#ffffff';
      ctx.beginPath();
      ctx.arc(-1.5, -1.5, 2, 0, Math.PI * 2);
      ctx.fill();

      ctx.restore();
      return;
    }

    // 通常色ぷよの2つの目
    const eyeSpacing = 8.5;
    const eyeY = -2;

    if (isClearing) {
      // 消滅前の驚き・耐え目 (＞ ＜) または まん丸見開き目
      ctx.strokeStyle = '#1a0933';
      ctx.lineWidth = 2.5;
      ctx.lineCap = 'round';

      // 左目 ＞
      ctx.beginPath();
      ctx.moveTo(-eyeSpacing - 4, eyeY - 4);
      ctx.lineTo(-eyeSpacing + 2, eyeY);
      ctx.lineTo(-eyeSpacing - 4, eyeY + 4);
      ctx.stroke();

      // 右目 ＜
      ctx.beginPath();
      ctx.moveTo(eyeSpacing + 4, eyeY - 4);
      ctx.lineTo(eyeSpacing - 2, eyeY);
      ctx.lineTo(eyeSpacing + 4, eyeY + 4);
      ctx.stroke();

      ctx.restore();
      return;
    }

    if (isBlinking) {
      // まばたき線 (- -)
      ctx.strokeStyle = '#1a0933';
      ctx.lineWidth = 2.5;
      ctx.lineCap = 'round';

      ctx.beginPath();
      ctx.moveTo(-eyeSpacing - 4, eyeY);
      ctx.lineTo(-eyeSpacing + 4, eyeY);
      ctx.moveTo(eyeSpacing - 4, eyeY);
      ctx.lineTo(eyeSpacing + 4, eyeY);
      ctx.stroke();

      ctx.restore();
      return;
    }

    // 開いた目
    [-eyeSpacing, eyeSpacing].forEach((xPos, idx) => {
      // 白目
      ctx.fillStyle = '#ffffff';
      ctx.beginPath();
      ctx.ellipse(xPos, eyeY, 5.5, 7, 0, 0, Math.PI * 2);
      ctx.fill();
      ctx.lineWidth = 1.2;
      ctx.strokeStyle = '#24103a';
      ctx.stroke();

      // 黒目 (瞳)
      ctx.fillStyle = '#1c0f2b';
      ctx.beginPath();
      const lookOffsetY = (eyeType === 'droopy') ? 1.5 : (eyeType === 'sharp' ? -1 : 0);
      ctx.arc(xPos, eyeY + lookOffsetY, 3.5, 0, Math.PI * 2);
      ctx.fill();

      // アイハイライト (キラキラ光彩)
      ctx.fillStyle = '#ffffff';
      ctx.beginPath();
      ctx.arc(xPos - 1.5, eyeY + lookOffsetY - 1.5, 1.6, 0, Math.PI * 2);
      ctx.fill();
    });

    ctx.restore();
  }

  /**
   * 盤面全体のレンダリング
   * @param {CanvasRenderingContext2D} ctx 
   * @param {Board} board 
   * @param {ParticleSystem} particleSystem 
   */
  renderBoard(ctx, board, particleSystem) {
    const width = ctx.canvas.width;
    const height = ctx.canvas.height;

    ctx.save();

    // 画面揺れオフセット
    if (this.shakeAmount > 0) {
      const sx = (Math.random() - 0.5) * this.shakeAmount;
      const sy = (Math.random() - 0.5) * this.shakeAmount;
      ctx.translate(sx, sy);
      this.shakeAmount *= 0.88;
      if (this.shakeAmount < 0.2) this.shakeAmount = 0;
    }

    // 背景クリア
    ctx.fillStyle = '#080a14';
    ctx.fillRect(0, 0, width, height);

    // グリッド線描画 (かすかなネオンガイドライン)
    ctx.strokeStyle = 'rgba(44, 53, 99, 0.25)';
    ctx.lineWidth = 1;
    for (let c = 1; c < 6; c++) {
      ctx.beginPath();
      ctx.moveTo(c * this.cellSize, 0);
      ctx.lineTo(c * this.cellSize, height);
      ctx.stroke();
    }
    for (let r = 1; r < 12; r++) {
      ctx.beginPath();
      ctx.moveTo(0, r * this.cellSize);
      ctx.lineTo(width, r * this.cellSize);
      ctx.stroke();
    }

    // 敗北ライン (3列目最上段 ×印の目印)
    const deadX = 2 * this.cellSize + this.cellSize / 2;
    const deadY = 0 * this.cellSize + this.cellSize / 2;
    ctx.save();
    ctx.strokeStyle = 'rgba(255, 42, 95, 0.4)';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(deadX - 8, deadY - 8);
    ctx.lineTo(deadX + 8, deadY + 8);
    ctx.moveTo(deadX + 8, deadY - 8);
    ctx.lineTo(deadX - 8, deadY + 8);
    ctx.stroke();
    ctx.restore();

    // 盤面上の固定ぷよの描画 (行1〜12。行0は隠し段)
    for (let r = 1; r <= 12; r++) {
      for (let c = 0; c < 6; c++) {
        const type = board.grid[r][c];
        if (type > 0) {
          const cx = c * this.cellSize + this.cellSize / 2;
          const cy = (r - 1) * this.cellSize + this.cellSize / 2;

          // 上下左右の同色接続チェック
          const connections = board.getConnections(r, c);

          // 消滅演出中かどうかの状態
          const isClearing = board.isClearingCell(r, c);
          const state = {
            shaking: isClearing,
            flash: isClearing
          };

          this.drawPuyo(ctx, cx, cy, type, connections, state);
        }
      }
    }

    // 操作中のぷよ（落下中ぷよ）の描画
    if (board.activePiece) {
      const piece = board.activePiece;
      const positions = piece.getCells();

      // ゴーストぷよ（着地予想ガイド）
      const ghostY = board.getGhostY(piece);
      const ghostPositions = piece.getCells(piece.x, ghostY, piece.rot);
      ctx.save();
      ctx.globalAlpha = 0.32;
      for (const pos of ghostPositions) {
        if (pos.y >= 1) {
          const gx = pos.x * this.cellSize + this.cellSize / 2;
          const gy = (pos.y - 1) * this.cellSize + this.cellSize / 2;
          this.drawPuyo(ctx, gx, gy, pos.type, {});
        }
      }
      ctx.restore();

      // 実体ぷよ
      for (const pos of positions) {
        if (pos.y >= 1) { // 隠し段より下のみ描画
          const cx = pos.x * this.cellSize + this.cellSize / 2;
          const cy = (pos.y - 1) * this.cellSize + this.cellSize / 2;
          this.drawPuyo(ctx, cx, cy, pos.type, {});
        }
      }
    }

    // パーティクル描画
    if (particleSystem) {
      particleSystem.draw(ctx);
    }

    ctx.restore();
  }

  /**
   * ネクストぷよ枠の描画 (NEXT & NEXT2)
   * @param {CanvasRenderingContext2D} ctx 
   * @param {Array} nextPairs 
   */
  renderNext(ctx, nextPairs) {
    if (!ctx || !nextPairs || nextPairs.length === 0) return;

    const w = ctx.canvas.width;
    const h = ctx.canvas.height;
    ctx.clearRect(0, 0, w, h);

    // 背景
    ctx.fillStyle = '#080a14';
    ctx.fillRect(0, 0, w, h);

    const pair1 = nextPairs[0]; // NEXT (主ぷよ + 子ぷよ)
    if (pair1) {
      const cx = w / 2;
      // 縦並びで中央描画
      this.drawPuyo(ctx, cx, 36, pair1.colorB, {});
      this.drawPuyo(ctx, cx, 84, pair1.colorA, {});
    }
  }

  /**
   * おじゃまぷよトレイのDOM更新
   * 小ぷよ(1), 大ぷよ(6), 岩ぷよ(30), 星ぷよ(180)
   */
  updateOjamaTray(elementId, ojamaCount) {
    const el = document.getElementById(elementId);
    if (!el) return;

    el.innerHTML = '';
    if (ojamaCount <= 0) return;

    let remaining = ojamaCount;
    const stars = Math.floor(remaining / 180);
    remaining %= 180;
    const rocks = Math.floor(remaining / 30);
    remaining %= 30;
    const larges = Math.floor(remaining / 6);
    remaining %= 6;
    const smalls = remaining;

    const addIcon = (className, text) => {
      const span = document.createElement('span');
      span.className = `ojama-icon ${className}`;
      span.textContent = text;
      el.appendChild(span);
    };

    for (let i = 0; i < Math.min(stars, 4); i++) addIcon('ojama-star', '★');
    for (let i = 0; i < Math.min(rocks, 5); i++) addIcon('ojama-rock', '◆');
    for (let i = 0; i < Math.min(larges, 5); i++) addIcon('ojama-large', '●');
    for (let i = 0; i < Math.min(smalls, 6); i++) addIcon('ojama-small', '・');

    if (ojamaCount > 300) {
      const countTag = document.createElement('span');
      countTag.style.fontSize = '0.65rem';
      countTag.style.color = '#ffbe0b';
      countTag.textContent = `+${ojamaCount}`;
      el.appendChild(countTag);
    }
  }
}

// シングルトン
const visualRenderer = new VisualRenderer();
