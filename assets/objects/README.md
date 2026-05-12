# Object Library

Add each OBJ model in its own folder:

```text
assets/objects/skull/skull.obj
assets/objects/saturn/saturn.obj
assets/objects/sea_turtle/sea_turtle.obj
assets/objects/heart/heart.obj
assets/objects/globe/globe.obj
```

Keep any `.mtl` files and referenced images in the same folder as the OBJ.
Then map marker IDs to those OBJ paths in `config/assets.json`. If the OBJ
does not reference its texture through an `.mtl` file, add a `texture` field
pointing directly at the image.
