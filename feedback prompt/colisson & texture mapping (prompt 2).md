# [x] Create src/CollisionSystem.h (AABB structs, resolvePlayer, raycastBoards)
# [ ] Modify src/board.h — add UV coordinates to vertex layout
# [ ] Modify src/main.cpp — integrate CollisionSystem, ExhibitionBoards, E key, G key
# Part 2: Texture & Board System
# [x] Create src/TextureManager.h (stb_image loader, checkerboard fallback)
# [x] Create src/ExhibitionBoard.h (6 boards, artwork, AABB, labels)
# [ ] Modify assets/shaders/papan.vs — add UV passthrough
# [ ] Modify assets/shaders/papan.fs — add artworkTex + highlighted uniform
# [ ] Modify assets/shaders/ruangan.fs — add worleyNoise, marbleTexture, brickTexture
# Verification
# [ ] Build check (CMake + compile)
# [ ] Runtime: wall collision + sliding
# [ ] Runtime: E key board interaction + highlight
# [ ] Runtime: artwork / checkerboard on boards
# [ ] Runtime: enhanced procedural textures visible



Artwork
Drop .jpg/.png files named artwork1.jpg–artwork6.jpg into assets/artwork/ to display real images. Missing files automatically get unique-coloured checkerboard fallbacks so the build never crashes.