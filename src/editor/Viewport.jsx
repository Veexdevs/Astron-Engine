import React, { useEffect, useRef } from 'react';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { useStore } from '../store.jsx';
import {
  Box, ChevronDown, Camera, Grid2x2, Move, Globe, Info, MoreVertical, Magnet,
} from 'lucide-react';

// ============ Viewport 3D (grade, eixos, cubo com gizmo) ============
export default function Viewport() {
  const mountRef = useRef(null);
  const apiRef = useRef(null);
  const { mode, playing, pushLog, pushToast, setSelection } = useStore();

  useEffect(() => {
    const mount = mountRef.current;
    if (!mount) return;

    let width = mount.clientWidth || 800;
    let height = mount.clientHeight || 500;

    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: false });
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    renderer.setSize(width, height);
    mount.appendChild(renderer.domElement);

    const scene = new THREE.Scene();

    // ---- céu em degradê (horizonte claro) ----
    const skyCanvas = document.createElement('canvas');
    skyCanvas.width = 4;
    skyCanvas.height = 256;
    const ctx = skyCanvas.getContext('2d');
    const grad = ctx.createLinearGradient(0, 0, 0, 256);
    grad.addColorStop(0, '#3f4247');
    grad.addColorStop(0.42, '#5c6067');
    grad.addColorStop(0.52, '#83878f');
    grad.addColorStop(0.56, '#4a4d53');
    grad.addColorStop(1, '#26272b');
    ctx.fillStyle = grad;
    ctx.fillRect(0, 0, 4, 256);
    const skyTex = new THREE.CanvasTexture(skyCanvas);
    skyTex.colorSpace = THREE.SRGBColorSpace;
    scene.background = skyTex;
    scene.fog = new THREE.Fog(0x6d717a, 55, 240);

    const camera = new THREE.PerspectiveCamera(55, width / height, 0.1, 1000);
    if (mode === '2d') {
      camera.position.set(0.01, 30, 0.01);
    } else {
      camera.position.set(7.2, 5.2, 9.4);
    }

    const controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.dampingFactor = 0.08;
    controls.target.set(0, mode === '2d' ? 0 : 0.9, 0);
    controls.maxPolarAngle = Math.PI / 2 - 0.02;
    controls.minDistance = 3;
    controls.maxDistance = 80;

    // ---- chão + grade ----
    const floor = new THREE.Mesh(
      new THREE.PlaneGeometry(500, 500),
      new THREE.MeshBasicMaterial({ color: 0x232427 })
    );
    floor.rotation.x = -Math.PI / 2;
    floor.position.y = -0.02;
    scene.add(floor);

    const grid = new THREE.GridHelper(400, 400, 0x43454b, 0x303136);
    grid.position.y = 0;
    scene.add(grid);

    // ---- eixos mundiais ----
    const axisLine = (to, color) => {
      const g = new THREE.BufferGeometry().setFromPoints([new THREE.Vector3(0, 0.01, 0), to]);
      const m = new THREE.LineBasicMaterial({ color });
      const line = new THREE.Line(g, m);
      scene.add(line);
    };
    axisLine(new THREE.Vector3(-200, 0.01, 0), 0xc93535);
    axisLine(new THREE.Vector3(200, 0.01, 0), 0xd8443c);
    axisLine(new THREE.Vector3(0, 0.01, -200), 0x2f6fe0);
    axisLine(new THREE.Vector3(0, 0.01, 200), 0x3b82f6);
    axisLine(new THREE.Vector3(0, 60, 0), 0x35c04a);

    // ---- iluminação ----
    scene.add(new THREE.AmbientLight(0xffffff, 0.75));
    const sun = new THREE.DirectionalLight(0xffffff, 1.1);
    sun.position.set(6, 12, 4);
    scene.add(sun);
    const fill = new THREE.DirectionalLight(0xbfd0ff, 0.35);
    fill.position.set(-8, 4, -6);
    scene.add(fill);

    // ---- cubo central ----
    const cube = new THREE.Mesh(
      new THREE.BoxGeometry(1.3, 1.3, 1.3),
      new THREE.MeshStandardMaterial({ color: 0xd7d7dc, roughness: 0.55, metalness: 0.08 })
    );
    cube.position.set(0, 0.66, 0);
    scene.add(cube);

    const edges = new THREE.LineSegments(
      new THREE.EdgesGeometry(cube.geometry),
      new THREE.LineBasicMaterial({ color: 0x8b8b93 })
    );
    cube.add(edges);

    // ---- gizmo de setas (vermelho X, verde Y, azul Z) ----
    const gizmo = new THREE.Group();
    const makeArrow = (dir, color) => {
      const grp = new THREE.Group();
      const mat = new THREE.MeshBasicMaterial({ color });
      const shaft = new THREE.Mesh(new THREE.CylinderGeometry(0.05, 0.05, 2.0, 12), mat);
      shaft.position.y = 1.0 + 0.4;
      const tip = new THREE.Mesh(new THREE.ConeGeometry(0.14, 0.42, 14), mat);
      tip.position.y = 2.0 + 0.4 + 0.21;
      grp.add(shaft, tip);
      grp.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), dir.clone().normalize());
      return grp;
    };
    gizmo.add(makeArrow(new THREE.Vector3(1, 0, 0), 0xe8443a));
    gizmo.add(makeArrow(new THREE.Vector3(0, 1, 0), 0x33d158));
    gizmo.add(makeArrow(new THREE.Vector3(0, 0, 1), 0x3b82f6));
    gizmo.position.set(0, 0.66, 0);
    scene.add(gizmo);

    // ---- malha auxiliar sutil do objeto selecionado ----
    const selBox = new THREE.LineSegments(
      new THREE.EdgesGeometry(new THREE.BoxGeometry(1.5, 1.5, 1.5)),
      new THREE.LineBasicMaterial({ color: 0x6aa4ff })
    );
    selBox.position.copy(cube.position);
    scene.add(selBox);

    // ---- loop ----
    let raf = 0;
    const clock = new THREE.Clock();
    const animate = () => {
      raf = requestAnimationFrame(animate);
      const t = clock.getElapsedTime();
      if (playing) {
        cube.position.y = 0.66 + Math.abs(Math.sin(t * 2.4)) * 0.7;
        gizmo.position.y = cube.position.y;
        selBox.position.y = cube.position.y;
      }
      controls.update();
      renderer.render(scene, camera);
    };
    animate();

    // ---- resize ----
    const ro = new ResizeObserver(() => {
      width = mount.clientWidth || width;
      height = mount.clientHeight || height;
      camera.aspect = width / height;
      camera.updateProjectionMatrix();
      renderer.setSize(width, height);
    });
    ro.observe(mount);

    // clique no cubo seleciona
    const ray = new THREE.Raycaster();
    const mouse = new THREE.Vector2();
    let downAt = null;
    const onDown = (e) => { downAt = { x: e.clientX, y: e.clientY }; };
    const onUp = (e) => {
      if (!downAt) return;
      if (Math.hypot(e.clientX - downAt.x, e.clientY - downAt.y) > 4) return;
      const rect = renderer.domElement.getBoundingClientRect();
      mouse.x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
      mouse.y = -((e.clientY - rect.top) / rect.height) * 2 + 1;
      ray.setFromCamera(mouse, camera);
      const hits = ray.intersectObject(cube);
      if (hits.length) {
        setSelection({ id: 'cube-1', label: 'Parte: Model', kind: 'object', props: 'model' });
        pushLog('INFO', 'Objeto "Parte: Model" selecionado no viewport.');
      }
    };
    renderer.domElement.addEventListener('pointerdown', onDown);
    renderer.domElement.addEventListener('pointerup', onUp);

    apiRef.current = { camera, controls };

    return () => {
      apiRef.current = null;
      cancelAnimationFrame(raf);
      ro.disconnect();
      renderer.domElement.removeEventListener('pointerdown', onDown);
      renderer.domElement.removeEventListener('pointerup', onUp);
      controls.dispose();
      renderer.dispose();
      mount.removeChild(renderer.domElement);
      scene.traverse((o) => {
        o.geometry?.dispose?.();
        if (o.material) {
          const mats = Array.isArray(o.material) ? o.material : [o.material];
          mats.forEach((m) => { m.map?.dispose?.(); m.dispose?.(); });
        }
      });
      skyTex.dispose();
    };
  }, [playing, mode, setSelection, pushLog]);

  return (
    <div className="viewport">
      <div ref={mountRef} className="viewport-canvas" />

      {/* toolbar do viewport */}
      <div className="vp-toolbar">
        <button className="vp-tbtn" title="Projeção"><Box size={15} /></button>
        <button className="vp-select">
          {mode === '2d' ? 'Ortográfica' : 'Perspectiva'} <ChevronDown size={12} />
        </button>
        <div className="vp-tgroup">
          <button className="vp-tbtn" title="Câmera"><Camera size={15} /></button>
          <button className="vp-tbtn" title="Grade"><Grid2x2 size={15} /></button>
          <button className="vp-tbtn" title="Mover"><Move size={15} /></button>
          <button className="vp-tbtn" title="Pivô"><Globe size={15} /></button>
          <button className="vp-tbtn" title="Snap"><Magnet size={15} /></button>
          <button className="vp-tbtn" title="Info" onClick={() => pushToast('Grade: 1un · Snap: ativo · Perspectiva')}>
            <Info size={15} />
          </button>
          <button className="vp-tbtn" title="Mais"><MoreVertical size={15} /></button>
        </div>
      </div>

      {/* gizmo de vistas (X/Y/Z) */}
      <div className="vp-gizmo">
        <svg width="92" height="92" viewBox="0 0 92 92">
          <line x1="46" y1="46" x2="46" y2="16" stroke="#33d158" strokeWidth="2" />
          <line x1="46" y1="46" x2="74" y2="62" stroke="#e8443a" strokeWidth="2" />
          <line x1="46" y1="46" x2="18" y2="62" stroke="#3b82f6" strokeWidth="2" />
          <circle cx="46" cy="14" r="11" fill="#33d158" />
          <text x="46" y="18.5" textAnchor="middle" fontSize="12" fontWeight="700" fill="#0b2d13">Y</text>
          <circle cx="76" cy="63" r="11" fill="#e8443a" />
          <text x="76" y="67.5" textAnchor="middle" fontSize="12" fontWeight="700" fill="#3a0b09">X</text>
          <circle cx="16" cy="63" r="11" fill="#3b82f6" />
          <text x="16" y="67.5" textAnchor="middle" fontSize="12" fontWeight="700" fill="#071a3d">Z</text>
          <circle cx="46" cy="46" r="5" fill="#cfd3da" />
          <circle cx="46" cy="80" r="6" fill="#3b82f6" opacity="0.85" />
        </svg>
      </div>

      {playing && (
        <div className="vp-playing">
          <span className="dot green" /> EXECUTANDO
        </div>
      )}
    </div>
  );
}
