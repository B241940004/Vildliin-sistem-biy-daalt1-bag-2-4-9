# II-4-9 · Designing a Thread Pool

F.CSM302 Үйлдлийн систем · Бие даалт 1 · Сэдэв 9 (Synchronization Examples, Operating System Concepts)
UI дүрслэл: **Worker activity**

Баг: B222270042, B232270097, B241940004, B241960045, B242270073

## Товч тайлбар
POSIX thread (pthread) ашигласан thread pool-ийг C хэлээр хэрэгжүүлсэн. Pool нь ажиллах явцдаа
submit / start / done / idle / rejected үйл явдлуудыг бүртгэнэ. Вэб UI нь тэр бодит логийг хугацаагаар нь
сэргээн тоглуулж, worker бүрийн төлөв (idle/running), task queue, цагийн диаграммыг харуулна.

## Бүтэц
| Файл | Үүрэг |
|---|---|
| `src/threadpool.h/.c` | Thread pool: `pool_init`, `pool_submit`, `pool_shutdown` |
| `src/tpdemo.c` | Pool-ийг ажиллуулж, JSON логийг stdout руу хэвлэнэ |
| `serve.py` | UI-г үйлчилж, параметрээр `tpdemo`-г ажиллуулна (зөвхөн Python 3 стандарт сан) |
| `ui/index.html` | Интерактив UI |
| `tests/run_tests.sh` | Автомат тест |

## Quick Start
Шаардлага: Linux/macOS (WSL), `gcc`, `make`, Python 3.
```
git clone https://github.com/Huslenc/Vildliin-sistem-biy-daalt1-bag-2-4-9.git
cd Vildliin-sistem-biy-daalt1-bag-2-4-9
make
make test
python3 serve.py
```
Дараа нь хөтчөөр http://localhost:8000 нээнэ.

## Thread pool-ийн дизайн
- Дараалал нь хязгаартай circular buffer (`queue`, `head`, `tail`, `count`).
- Нэг mutex дараалал болон shutdown флагийг хамгаална; `not_empty` condition variable-ээр worker-ууд хүлээнэ.
- `pool_submit` дараалал дүүрсэн бол task-ийг авахгүй, `1` буцаана (UI дээр "татгалзал" болж харагдана).
- Task-ийг mutex-ийн гадна ажиллуулдаг тул олон worker зэрэг ажиллана.
- `pool_shutdown` дараалалд үлдсэн бүх task дуусахыг хүлээж, worker-уудыг `join` хийнэ.

## Параметр
workers 1–16, queue 1–32, tasks 1–200, task хугацаа 1–5000 мс, submit завсар 0–2000 мс.
Буруу утга оруулбал сервер алдааны мессеж буцаана.

## Тест
`make test`: 1 worker, ердийн ажиллагаа, жижиг queue (back-pressure), worker > queue, 1 task, буруу оролт.
Тест бүр "бүх task дууссан, `done` лог task бүрт бичигдсэн, зэрэг ажилласан worker ≤ worker-ийн тоо" гэдгийг шалгана.

## Дэлгэцийн зураг
Туршсан орчин: Debian, gcc 14.2.0, Python 3.13.5 (`docs/screenshots/05_environment.png`).

**1. Бүх task дууссан** (5 worker, queue 6, 20 task, submit завсар 40 мс). 20/20 task дууссан, worker ашиглалт 88 %,
queue дүүрсэн үед 5 task хүлээлгэгдсэн, C шалгалт OK (зэрэг ажилласан worker дээд тал нь 5).
![completed](docs/screenshots/01_completed_5workers.png)

**2. Ажиллаж байх үе** (1.31 с). 5 worker бүгд running, queue 6/6 дүүрсэн.
![running](docs/screenshots/02_running_queue_full.png)

**3. Жижиг queue** (2 worker, queue 3, 20 task, submit завсар 10 мс). 15 task дараалал дүүрсэн үед хүлээлгэгдсэн,
worker ашиглалт 99 %, дундаж хүлээлт 620 мс.
![small queue](docs/screenshots/03_small_queue_2workers.png)

**4. Автомат тест** (`make test`).
![tests](docs/screenshots/04_make_test.png)

## Known issues
- Task нь `usleep`-ээр симуляц хийсэн ажил; бодит тооцоолол биш.
- Windows дээр шууд ажиллахгүй (WSL ашиглана).

## Ашигласан материал
- Silberschatz et al., *Operating System Concepts* (2018), Programming Projects: Designing a Thread Pool.
- AI ашигласан: Claude (Anthropic). UI болон README-ийн ноорогийг AI-аар үүсгэсэн.
  Баг кодыг өөрсдөө build/run хийж, тестлэж, screenshot-оор илэрсэн алдааг (лог буфер дүүрч үйл явдал алдагддаг байсан) засуулсан.
