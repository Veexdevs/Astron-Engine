// ============ Astron Engine — dados iniciais (seed) ============

export const SEED_PROJECTS = [
  { id: 'meu-jogo', name: 'Meu Jogo', created: '22/11/2026', thumb: '/thumb-cube.jpg' },
  { id: 'projeto-rp', name: 'Projeto RP', created: '21/11/2026', thumb: '/thumb-city.jpg' },
  { id: 'obby-teste', name: 'Obby Teste', created: '20/11/2026', thumb: '/thumb-obby.jpg' },
];

export const TEMPLATE_OPTIONS = [
  { id: 'blank', label: 'Em Branco', desc: 'Cena vazia em C#' },
  { id: 'platformer', label: 'Plataforma 3D', desc: 'Player, câmera e cenário' },
  { id: 'obby', label: 'Obby', desc: 'Plataformas flutuantes' },
  { id: 'topdown', label: 'Top-Down', desc: 'Visão superior com inimigos' },
];

const MAIN_CS = `using Astron.Engine;

// Ponto de entrada do jogo — gerado pelo Astron Engine
public static class Main
{
    public static void Main(string[] args)
    {
        using var game = new GameWindow(1280, 720, "Meu Jogo");
        game.LoadScene("res://default.scene");
        game.Run();
    }
}
`;

const PLAYER_CS = `using Astron.Engine;

// Controla o movimento e o pulo do jogador
public class Player : Component
{
    public float Speed = 320f;
    public float JumpPower = 720f;

    private bool _grounded;
    private float _coyote;

    public override void Update(float dt)
    {
        float axis = Input.GetAxis("Horizontal"); // -1 .. 1
        Transform.Velocity.X = axis * Speed;

        _coyote = _grounded ? 0.12f : _coyote - dt;

        if (Input.JumpPressed && _coyote > 0f)
        {
            Transform.Velocity.Y = -JumpPower;
            _grounded = false;
            _coyote = 0f;
            Audio.Play("res://sounds/jump.wav");
        }

        _grounded = Physics.IsGrounded(this);
    }
}
`;

const GAME_CS = `using Astron.Engine;

// Regra do jogo: coletar pickups e pontuar
public class Game : Scene
{
    public int Score { get; private set; }

    public override void OnStart()
    {
        Spawn("Player", new Vector2(120f, 400f));
        HUD.SetText("Pontos: 0");
    }

    public override void OnPickupCollected(Pickup pickup)
    {
        Score += 10;
        HUD.SetText($"Pontos: {Score}");
        Audio.Play("res://sounds/pickup.wav");
    }
}
`;

const SCENE_JSON = `{
  "name": "Cena Principal",
  "engine": "Astron Engine 1.0.0",
  "gravity": 1800,
  "nodes": [
    {
      "id": "mundo-3d",
      "type": "World3D",
      "children": [
        { "id": "objetos", "type": "Group", "children": [] },
        { "id": "terreno", "type": "Terrain", "size": [256, 256] },
        { "id": "iluminacao", "type": "DirectionalLight", "rotation": [-45, 30, 0] }
      ]
    },
    { "id": "cena-principal", "type": "Scene", "res": "res://default.scene" },
    { "id": "player", "type": "Prefab", "script": "res://scripts/Player.cs", "position": [120, 400, 0] },
    { "id": "game", "type": "Script", "script": "res://scripts/Game.cs" },
    { "id": "camera", "type": "Camera", "fov": 60, "position": [0, 240, -420] }
  ]
}
`;

const CUBE_MODEL = `# Astron Model — cube.model
o Cube
v -0.5 -0.5  0.5
v  0.5 -0.5  0.5
v  0.5  0.5  0.5
v -0.5  0.5  0.5
v -0.5 -0.5 -0.5
v  0.5 -0.5 -0.5
v  0.5  0.5 -0.5
v -0.5  0.5 -0.5
f 1 2 3 4
f 8 7 6 5
f 4 3 7 8
f 5 1 4 8
f 2 6 7 3
f 6 2 1 5
`;

const CHECKER_TEX = `// checker.tex — textura procedural do Astron Engine
size: 64x64
tiles: 8
color_a: "#e8e8ec"
color_b: "#c9c9cf"
filter: point
`;

const JUMP_WAV = `; jump.wav (placeholder binário exportado pelo editor de áudio)
; 22050 Hz · 16-bit · mono · 0.18s
RIFF....WAVEfmt ....data ....
`;

const HUD_UI = `{
  "name": "HUD",
  "root": {
    "type": "Canvas",
    "children": [
      { "type": "Label", "id": "score", "text": "Pontos: 0", "anchor": "top-left", "margin": [24, 24] },
      { "type": "TextureRect", "id": "heart", "anchor": "top-right", "margin": [24, 24] }
    ]
  }
}
`;

// Árvore padrão do painel Explorer (idêntica ao layout do editor)
export const EXPLORER_TREE = [
  {
    id: 'projeto',
    label: 'Projeto',
    icon: 'box',
    children: [
      {
        id: 'cenas',
        label: 'Cenas',
        icon: 'folder',
        children: [
          { id: 'cena-principal', label: 'Cena Principal', icon: 'scene', kind: 'scene', color: '#58a6ff' },
          { id: 'cena-secundaria', label: 'Cena Secundária', icon: 'scene2', kind: 'scene', color: '#58a6ff' },
        ],
      },
      {
        id: 'mundo-3d',
        label: 'Mundo 3D',
        icon: 'folder',
        children: [
          { id: 'objetos', label: 'Objetos', icon: 'objects', color: '#58a6ff', dynamic: 'objects' },
          { id: 'terreno', label: 'Terreno', icon: 'mountain' },
          { id: 'iluminacao', label: 'Iluminação', icon: 'bulb', color: '#e3b341' },
        ],
      },
      { id: 'cameras', label: 'Câmeras', icon: 'camera', children: [
        { id: 'cam-principal', label: 'Câmera Principal' },
        { id: 'cam-fps', label: 'Câmera FPS' },
      ] },
      { id: 'personagens', label: 'Personagens', icon: 'user', children: [
        { id: 'player-node', label: 'Player' },
      ] },
      { id: 'fisica', label: 'Física', icon: 'activity', children: [
        { id: 'gravidade-global', label: 'Gravidade Global' },
        { id: 'colisores', label: 'Colisores' },
      ] },
      { id: 'audio', label: 'Audio', icon: 'music', children: [
        { id: 'musica-theme', label: 'Música Theme' },
        { id: 'efeitos', label: 'Efeitos' },
      ] },
      { id: 'interface', label: 'Interface', icon: 'monitor', children: [
        { id: 'hud-node', label: 'HUD' },
      ] },
      { id: 'scripts', label: 'Scripts', icon: 'code', color: '#7ee787', children: [
        { id: 'sc-player', label: 'Player.cs', icon: 'filecode', path: 'res://scripts/Player.cs', color: '#7ee787' },
        { id: 'sc-game', label: 'Game.cs', icon: 'filecode', path: 'res://scripts/Game.cs', color: '#7ee787' },
      ] },
      { id: 'recursos', label: 'Recursos', icon: 'folder', children: [
        { id: 'sprites', label: 'Sprites' },
        { id: 'fontes', label: 'Fontes' },
      ] },
      { id: 'materiais', label: 'Materiais', icon: 'circle', children: [
        { id: 'mat-plastic', label: 'Plastic' },
        { id: 'mat-metal', label: 'Metal' },
      ] },
    ],
  },
];

// Ordem padrão das pastas/raiz do painel Arquivos (res://)
export const FOLDER_ORDER = ['models', 'textures', 'scripts', 'sounds', 'ui'];
export const ROOT_FILE_ORDER = ['default.scene', 'main.cs'];

export function defaultFiles(projectName = 'Meu Jogo') {
  return {
    'res://default.scene': SCENE_JSON,
    'res://main.cs': MAIN_CS.replace('"Meu Jogo"', `"${projectName}"`),
    'res://scripts/Player.cs': PLAYER_CS,
    'res://scripts/Game.cs': GAME_CS,
    'res://models/cube.model': CUBE_MODEL,
    'res://textures/checker.tex': CHECKER_TEX,
    'res://sounds/jump.wav': JUMP_WAV,
    'res://ui/hud.ui': HUD_UI,
  };
}

export const SEED_LOGS = [
  { level: 'INFO', msg: 'Projeto carregado com sucesso!' },
  { level: 'WARNING', msg: 'Nenhum objeto selecionado.' },
  { level: 'ERROR', msg: 'Erro ao carregar o script principal.' },
];

export const REPO_URL = 'https://github.com/Veexdevs/Astron-Engine';
export const ACTIONS_URL = 'https://github.com/Veexdevs/Astron-Engine/actions';
