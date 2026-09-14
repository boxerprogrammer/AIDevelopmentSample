/**
 * js/main.js
 * エントリーポイント、localStorageランキング管理、UIイベントハンドラ配線
 */

const STORAGE_KEY = 'PUYO_CLASH_HIGHSCORES';

// デフォルトのアーケードランキングレコード
const DEFAULT_RANKINGS = [
  { name: 'ARLE', score: 85000, stage: 5, maxChain: 7, date: '2026/09/01' },
  { name: 'RULUE', score: 62000, stage: 4, maxChain: 5, date: '2026/09/05' },
  { name: 'SCHEZO', score: 48000, stage: 3, maxChain: 4, date: '2026/09/08' },
  { name: 'DRACO', score: 29000, stage: 2, maxChain: 3, date: '2026/09/10' },
  { name: 'SUKE', score: 14000, stage: 1, maxChain: 2, date: '2026/09/11' }
];

class RankingManager {
  static load() {
    try {
      const data = localStorage.getItem(STORAGE_KEY);
      if (data) {
        return JSON.parse(data);
      }
    } catch (e) {
      console.warn('LocalStorage error:', e);
    }
    return [...DEFAULT_RANKINGS];
  }

  static save(rankings) {
    try {
      localStorage.setItem(STORAGE_KEY, JSON.stringify(rankings.slice(0, 5)));
    } catch (e) {
      console.warn('LocalStorage save error:', e);
    }
  }

  static reset() {
    try {
      localStorage.removeItem(STORAGE_KEY);
    } catch (e) {
      console.warn('LocalStorage reset error:', e);
    }
    return [...DEFAULT_RANKINGS];
  }

  static addRecord(name, score, stage, maxChain) {
    const rankings = this.load();
    const now = new Date();
    const dateStr = `${now.getFullYear()}/${String(now.getMonth() + 1).padStart(2, '0')}/${String(now.getDate()).padStart(2, '0')}`;

    rankings.push({
      name: (name || 'PLAYER 1').toUpperCase().slice(0, 10),
      score: score,
      stage: stage,
      maxChain: maxChain,
      date: dateStr
    });

    // スコア降順ソート
    rankings.sort((a, b) => b.score - a.score);
    const top5 = rankings.slice(0, 5);
    this.save(top5);
    return top5;
  }
}

// アプリケーション初期化
document.addEventListener('DOMContentLoaded', () => {
  const game = new GameManager();

  // 初期レンダリング
  game.render();

  // =========================================================================
  // UIイベントハンドラー配線
  // =========================================================================

  // サウンド切り替え
  const soundBtn = document.getElementById('btn-sound-toggle');
  soundBtn.addEventListener('click', () => {
    const unmuted = soundEngine.toggleMute();
    soundBtn.textContent = unmuted ? '🔊 BGM/SE' : '🔇 MUTED';
    soundBtn.style.color = unmuted ? '#fff' : '#ff477e';
  });

  // ポーズボタン
  const pauseBtn = document.getElementById('btn-pause');
  pauseBtn.addEventListener('click', () => {
    game.togglePause();
  });

  const resumeBtn = document.getElementById('btn-resume');
  resumeBtn.addEventListener('click', () => {
    game.togglePause();
  });

  const pauseToTitleBtn = document.getElementById('btn-pause-to-title');
  pauseToTitleBtn.addEventListener('click', () => {
    document.getElementById('pause-modal').classList.remove('active');
    game.gameState = 'TITLE';
    soundEngine.stopBGM();
    document.getElementById('title-modal').classList.add('active');
  });

  // 操作説明モーダル
  const howtoModal = document.getElementById('howto-modal');
  const openHowtoBtn = document.getElementById('btn-how-to');
  const titleHowtoBtn = document.getElementById('btn-title-howto');
  const closeHowtoBtn = document.getElementById('btn-close-howto');

  const openHowto = () => {
    soundEngine.resume();
    howtoModal.classList.add('active');
  };
  openHowtoBtn.addEventListener('click', openHowto);
  titleHowtoBtn.addEventListener('click', openHowto);
  closeHowtoBtn.addEventListener('click', () => {
    howtoModal.classList.remove('active');
  });

  // ランキングモーダル
  const rankingModal = document.getElementById('ranking-modal');
  const openRankingBtn = document.getElementById('btn-ranking-open');
  const titleRankingBtn = document.getElementById('btn-title-ranking');
  const closeRankingBtn = document.getElementById('btn-close-ranking');
  const resetRankingBtn = document.getElementById('btn-reset-ranking');

  const renderRankingTable = () => {
    const list = RankingManager.load();
    const tbody = document.getElementById('ranking-table-body');
    tbody.innerHTML = '';

    list.forEach((item, index) => {
      const tr = document.createElement('tr');
      const rankBadge = index === 0 ? '🥇 1st' : (index === 1 ? '🥈 2nd' : (index === 2 ? '🥉 3rd' : `${index + 1}th`));
      tr.innerHTML = `
        <td style="color: #ffbe0b; font-weight: bold;">${rankBadge}</td>
        <td>${item.name}</td>
        <td style="color: #00f0ff;">${item.score.toLocaleString()}</td>
        <td>Stage ${item.stage}</td>
        <td>${item.maxChain} Ch</td>
        <td style="color: #8b9bb4;">${item.date}</td>
      `;
      tbody.appendChild(tr);
    });
  };

  const openRanking = () => {
    soundEngine.resume();
    renderRankingTable();
    rankingModal.classList.add('active');
  };

  openRankingBtn.addEventListener('click', openRanking);
  titleRankingBtn.addEventListener('click', openRanking);
  closeRankingBtn.addEventListener('click', () => {
    rankingModal.classList.remove('active');
  });

  resetRankingBtn.addEventListener('click', () => {
    if (confirm('ハイスコアランキングを初期化しますか？')) {
      RankingManager.reset();
      renderRankingTable();
    }
  });

  // ゲーム開始 (タイトル画面 -> Stage 1)
  const titleModal = document.getElementById('title-modal');
  const startGameBtn = document.getElementById('btn-start-game');
  startGameBtn.addEventListener('click', () => {
    soundEngine.resume();
    titleModal.classList.remove('active');
    game.startStage(1);
  });

  // リザルト画面アクション
  const resultModal = document.getElementById('result-modal');
  const nextStageBtn = document.getElementById('btn-next-stage');
  const retryBtn = document.getElementById('btn-retry');
  const titleReturnBtn = document.getElementById('btn-title-return');

  // スコア保存ヘルパー
  const saveCurrentScore = () => {
    const nameInput = document.getElementById('player-name-input');
    const name = nameInput.value.trim() || 'PLAYER 1';
    RankingManager.addRecord(name, game.playerBoard.score, game.currentStage, game.playerBoard.maxChain);
  };

  // 次のステージへ
  nextStageBtn.addEventListener('click', () => {
    soundEngine.resume();
    saveCurrentScore();
    resultModal.classList.remove('active');
    const nextStage = game.currentStage + 1;
    game.startStage(nextStage);
  });

  // もう一度挑戦 (Stage 1からやり直し)
  retryBtn.addEventListener('click', () => {
    soundEngine.resume();
    saveCurrentScore();
    resultModal.classList.remove('active');
    game.startStage(1);
  });

  // タイトルへ戻る
  titleReturnBtn.addEventListener('click', () => {
    soundEngine.resume();
    saveCurrentScore();
    resultModal.classList.remove('active');
    game.gameState = 'TITLE';
    soundEngine.stopBGM();
    titleModal.classList.add('active');
  });
});
