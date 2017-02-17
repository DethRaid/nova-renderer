package com.continuum.nova;

import com.continuum.nova.NovaNative.window_size;
import com.continuum.nova.chunks.ChunkUpdateListener;
import com.continuum.nova.gui.NovaDraw;
import com.continuum.nova.utils.AtlasGenerator;
import com.continuum.nova.utils.Utils;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.ScaledResolution;
import net.minecraft.client.renderer.texture.TextureAtlasSprite;
import net.minecraft.client.renderer.texture.TextureMap;
import net.minecraft.client.resources.IResource;
import net.minecraft.client.resources.IResourceManager;
import net.minecraft.client.resources.IResourceManagerReloadListener;
import net.minecraft.util.ResourceLocation;
import net.minecraft.world.World;
import net.minecraft.world.chunk.IChunkProvider;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import javax.annotation.Nonnull;
import javax.imageio.ImageIO;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.BufferedInputStream;
import java.io.IOException;
import java.lang.management.ManagementFactory;
import java.util.*;
import java.util.List;

public class NovaRenderer implements IResourceManagerReloadListener {
    public static final String MODID = "Nova Renderer";
    public static final String VERSION = "0.0.3";

    private static final Logger LOG = LogManager.getLogger(NovaRenderer.class);
    public static final ResourceLocation WHITE_TEXTURE_GUI_LOCATION = new ResourceLocation("white_gui");

    private boolean firstLoad = true;

    private static final List<ResourceLocation> GUI_ALBEDO_TEXTURES_LOCATIONS = new ArrayList<>();
    private TextureMap guiAtlas = new TextureMap("textures");
    private Map<ResourceLocation, TextureAtlasSprite> guiSpriteLocations = new HashMap<>();

    private static final List<ResourceLocation> TERRAIN_ALBEDO_TEXTURES_LOCATIONS = new ArrayList<>();
    private TextureMap blockAtlas = new TextureMap("textures");
    private Map<ResourceLocation, TextureAtlasSprite> blockSpriteLocations = new HashMap<>();

    private static final List<ResourceLocation> FONT_ALBEDO_TEXTURES_LOCATIONS = new ArrayList<>();
    private TextureMap fontAtlas = new TextureMap("textures");
    private Map<ResourceLocation, TextureAtlasSprite> fontSpriteLocations = new HashMap<>();

    private static final List<ResourceLocation> FREE_TEXTURES = new ArrayList<>();

    private int height;
    private int width;

    private boolean resized;
    private int scalefactor;

    private IResourceManager resourceManager;

    private World world;

    private ChunkUpdateListener chunkUpdateListener;

    public NovaRenderer() {
        // I put these in Utils to make this class smaller
        Utils.initBlockTextureLocations(TERRAIN_ALBEDO_TEXTURES_LOCATIONS);
        Utils.initGuiTextureLocations(GUI_ALBEDO_TEXTURES_LOCATIONS);
        Utils.initFontTextureLocations(FONT_ALBEDO_TEXTURES_LOCATIONS);
        Utils.initFreeTextures(FREE_TEXTURES);
    }

    @Override
    public void onResourceManagerReload(@Nonnull IResourceManager resourceManager) {
        this.resourceManager = resourceManager;

        if (firstLoad) {
            firstLoad = false;
        }

        NovaNative.INSTANCE.reset_texture_manager();
        int maxAtlasSize = NovaNative.INSTANCE.get_max_texture_size();
        addTextures(TERRAIN_ALBEDO_TEXTURES_LOCATIONS, NovaNative.BLOCK_COLOR_ATLAS_NAME, resourceManager, maxAtlasSize);
        LOG.debug("Created block color atlas");

        addGuiAtlas(resourceManager);
        addFontAtlas(resourceManager);
        addFreeTextures(resourceManager);
    }

    /**
     * Adds the texutres that just hang out without a texture atlas
     *
     * @param resourceManager
     */
    private void addFreeTextures(IResourceManager resourceManager) {
        for (ResourceLocation loc : FREE_TEXTURES) {
            try {
                IResource texture = resourceManager.getResource(loc);
                BufferedInputStream in = new BufferedInputStream(texture.getInputStream());
                BufferedImage image = ImageIO.read(in);
                if (image != null) {
                    loadTexture(loc, image);
                } else {
                    LOG.error("Free texture " + loc + " has no data!");
                }
            } catch (IOException e) {
                LOG.error("Could not load free texture " + loc, e);
            }
        }
    }

    private void addGuiAtlas(@Nonnull IResourceManager resourceManager) {
        guiAtlas.createWhiteTexture(WHITE_TEXTURE_GUI_LOCATION);
        addAtlas(resourceManager, guiAtlas, GUI_ALBEDO_TEXTURES_LOCATIONS, guiSpriteLocations, NovaNative.GUI_ATLAS_NAME);
        LOG.debug("Created GUI atlas");
    }

    private void addFontAtlas(@Nonnull IResourceManager resourceManager) {
        addAtlas(resourceManager, fontAtlas, FONT_ALBEDO_TEXTURES_LOCATIONS, fontSpriteLocations, NovaNative.FONT_ATLAS_NAME);
        LOG.debug("Created font atlas");
    }

    private void addAtlas(@Nonnull IResourceManager resourceManager, TextureMap atlas, List<ResourceLocation> resoruces,
                          Map<ResourceLocation, TextureAtlasSprite> spriteLocations, String textureName) {
        atlas.loadSprites(resourceManager, textureMapIn -> resoruces.forEach(location -> {
            TextureAtlasSprite textureAtlasSprite = textureMapIn.registerSprite(location);
            spriteLocations.put(location, textureAtlasSprite);
        }));

        Optional<TextureAtlasSprite> whiteImage = atlas.getWhiteImage();
        whiteImage.ifPresent(image -> spriteLocations.put(image.getLocation(), image));

        NovaNative.mc_atlas_texture atlasTexture = getFullImage(atlas.getWidth(), atlas.getHeight(), spriteLocations.values());
        atlasTexture.setName(textureName);

        NovaNative.INSTANCE.add_texture(atlasTexture);

        for (TextureAtlasSprite sprite : spriteLocations.values()) {
            NovaNative.mc_texture_atlas_location location = new NovaNative.mc_texture_atlas_location(
                    sprite.getIconName(),
                    sprite.getMinU(),
                    sprite.getMinV(),
                    sprite.getMaxU(),
                    sprite.getMaxV()
            );

            LOG.info("Adding a sprite with name " + sprite.getIconName());

            NovaNative.INSTANCE.add_texture_location(location);
        }
    }

    private NovaNative.mc_atlas_texture getFullImage(int atlasWidth, int atlasHeight, Collection<TextureAtlasSprite> sprites) {
        byte[] imageData = new byte[atlasWidth * atlasHeight * 4];

        for (TextureAtlasSprite sprite : sprites) {
            LOG.debug("Looking at sprite " + sprite.getIconName());
            int startY = sprite.getOriginY() * atlasWidth * 4;
            int startPos = sprite.getOriginX() * 4 + startY;

            int[] data = sprite.getFrameTextureData(0)[0];
            for (int y = 0; y < sprite.getIconHeight(); y++) {
                for (int x = 0; x < sprite.getIconWidth(); x++) {
                    // Reverse the order of the color channels
                    int pixel = data[y * sprite.getIconWidth() + x];

                    byte red = (byte) (pixel & 0xFF);
                    byte green = (byte) ((pixel >> 8) & 0xFF);
                    byte blue = (byte) ((pixel >> 16) & 0xFF);
                    byte alpha = (byte) ((pixel >> 24) & 0xFF);

                    int imageDataBasePos = startPos + x * 4 + y * atlasWidth * 4;
                    imageData[imageDataBasePos] = blue;
                    imageData[imageDataBasePos + 1] = green;
                    imageData[imageDataBasePos + 2] = red;
                    imageData[imageDataBasePos + 3] = alpha;
                }
            }
        }

        return new NovaNative.mc_atlas_texture(
                atlasWidth,
                atlasHeight,
                4,
                imageData
        );
    }

    private void addTextures(List<ResourceLocation> locations, String textureName, IResourceManager resourceManager, int maxAtlasSize) {
        AtlasGenerator gen = new AtlasGenerator();
        List<AtlasGenerator.ImageName> images = new ArrayList<>();

        for (ResourceLocation textureLocation : locations) {
            try {
                IResource texture = resourceManager.getResource(textureLocation);
                BufferedInputStream in = new BufferedInputStream(texture.getInputStream());
                BufferedImage image = ImageIO.read(in);

                if (image != null) {
                    images.add(new AtlasGenerator.ImageName(image, textureLocation.toString()));
                }
            } catch (IOException e) {
                LOG.warn("IOException when loading texture " + textureLocation.toString(), e);
            }
        }

        List<AtlasGenerator.Texture> atlases = gen.Run(maxAtlasSize, maxAtlasSize, 0, images);

        for (AtlasGenerator.Texture texture : atlases) {
            try {
                BufferedImage image = texture.getImage();

                byte[] imageData = getImageData(image);

                NovaNative.mc_atlas_texture atlasTex = new NovaNative.mc_atlas_texture(
                        image.getWidth(),
                        image.getHeight(),
                        image.getColorModel().getNumComponents(),
                        imageData
                );
                atlasTex.setName(textureName);
                NovaNative.INSTANCE.add_texture(atlasTex);
                Map<String, Rectangle> rectangleMap = texture.getRectangleMap();

                for (String texName : rectangleMap.keySet()) {
                    Rectangle rect = rectangleMap.get(texName);
                    NovaNative.mc_texture_atlas_location atlasLoc = new NovaNative.mc_texture_atlas_location(
                            texName,
                            rect.x / (float) image.getWidth(),
                            rect.y / (float) image.getHeight(),
                            rect.width / (float) image.getWidth(),
                            rect.height / (float) image.getHeight()
                    );
                    NovaNative.INSTANCE.add_texture_location(atlasLoc);
                }
            } catch (AtlasGenerator.Texture.WrongNumComponentsException e) {
                LOG.error("Could not process a texture", e);
            }
        }
    }

    private byte[] getImageData(BufferedImage image) {

        byte[] convertedImageData = new byte[image.getWidth()*image.getHeight()*4];
            int counter = 0;
            for (int y = 0; y < image.getHeight(); y ++) {
                    for (int x = 0;x<image.getWidth();x++) {

                        Color c = new Color(image.getRGB(x,y),image.getColorModel().hasAlpha());

                        convertedImageData[counter] =(byte) (c.getRed());
                        convertedImageData[counter + 1] = (byte)(c.getGreen());
                        convertedImageData[counter + 2] = (byte)(c.getBlue());
                        convertedImageData[counter + 3] = (byte) (image.getColorModel().getNumComponents() == 3 ? 255 : c.getAlpha());
                        counter+=4;
                    }
            }
            return convertedImageData;



    }

    public void preInit() {
        System.getProperties().setProperty("jna.library.path", System.getProperty("java.library.path"));
        System.getProperties().setProperty("jna.dump_memory", "false");
        String pid = ManagementFactory.getRuntimeMXBean().getName();
        LOG.info("PID: " + pid);
        NovaNative.INSTANCE.initialize();
        LOG.info("Native code initialized");
        updateWindowSize();

        // Moved here so that it's initialized after the native code is loaded
        chunkUpdateListener  = new ChunkUpdateListener();
    }

    private void updateWindowSize() {
        window_size size = NovaNative.INSTANCE.get_window_size();
        int oldHeight = height;
        int oldWidth = width;
        if (oldHeight != size.height || oldWidth != size.width) {
            resized = true;
        } else {
            resized = false;
        }
        height = size.height;
        width = size.width;

    }

    public int getHeight() {
        return height;
    }

    public int getWidth() {
        return width;
    }

    public boolean wasResized() {
        return resized;
    }

    public void updateCameraAndRender(float renderPartialTicks, long systemNanoTime, Minecraft mc) {
        if (NovaNative.INSTANCE.should_close()) {
            Minecraft.getMinecraft().shutdown();
        }

        if (Minecraft.getMinecraft().currentScreen != null) {

            NovaDraw.novaDrawScreen(Minecraft.getMinecraft().currentScreen, renderPartialTicks);

        }

        NovaNative.INSTANCE.execute_frame();
        updateWindowSize();
        int scalefactor = new ScaledResolution(Minecraft.getMinecraft()).getScaleFactor() * 2;
        if (scalefactor != this.scalefactor) {
            NovaNative.INSTANCE.set_float_setting("scalefactor", scalefactor);
            this.scalefactor = scalefactor;
        }
    }

    public static String atlasTextureOfSprite(ResourceLocation texture) {
        ResourceLocation strippedLocation = new ResourceLocation(texture.getResourceDomain(), texture.getResourcePath().replace(".png", "").replace("textures/", ""));

        if (TERRAIN_ALBEDO_TEXTURES_LOCATIONS.contains(strippedLocation)) {
            return NovaNative.BLOCK_COLOR_ATLAS_NAME;
        } else if (GUI_ALBEDO_TEXTURES_LOCATIONS.contains(strippedLocation) || texture == WHITE_TEXTURE_GUI_LOCATION) {
            return NovaNative.GUI_ATLAS_NAME;
        } else if (FONT_ALBEDO_TEXTURES_LOCATIONS.contains(strippedLocation)) {
            return NovaNative.FONT_ATLAS_NAME;
        }

        return texture.toString();
    }

    public void setWorld(World world) {
        if(world != null) {
            this.world = world;
            chunkUpdateListener.setWorld(world);

            this.world.addEventListener(chunkUpdateListener);
            loadChunks();
        }
    }

    private void loadChunks() {
        IChunkProvider chunkProvider = world.getChunkProvider();
    }

    /**
     * Loads the specified texture, adding it to Minecraft as a texture outside of an atlas
     *
     * @param location The location of the texture
     * @param image    The texture itself
     */
    public void loadTexture(ResourceLocation location, BufferedImage image) {
        if (resourceManager == null) {
            LOG.error("Trying to load texture " + location + " but there's no resource manager");
            return;
        }

        byte[] imageData = getImageData(image);

        NovaNative.mc_atlas_texture tex = new NovaNative.mc_atlas_texture(image.getWidth(), image.getHeight(), 4, imageData);
        tex.setName(location.toString());
        NovaNative.INSTANCE.add_texture(tex);

        NovaNative.mc_texture_atlas_location loc = new NovaNative.mc_texture_atlas_location(location.toString(), 0, 0, 1, 1);
        NovaNative.INSTANCE.add_texture_location(loc);
    }
}
