#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  
in vec2 TexCoords;
  
// Güneş / Ay Işığı (Yönlü/Genel Işık)
uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;

uniform sampler2D diffuseMap;
uniform bool useTexture;
uniform float specularStrength;
uniform float shininess;
uniform float objectAlpha; // Saydamlık kontrolü (duman için)
uniform vec2 uvScale;      // Doku ölçekleme (tiling) kontrolü
uniform vec2 uvOffset;     // Doku kaydırma (animasyon) kontrolü

// Kapı Üstü Lambası (Noktasal Işık)
uniform bool lampOn;
uniform vec3 lampPos;
uniform vec3 lampColor;

// Şömine Ateşi (Flickering Noktasal Işık)
uniform vec3 firePos;
uniform vec3 fireColor;

// Sol Oda Lambası (Noktasal Işık)
uniform vec3 roomLightLeftPos;
uniform vec3 roomLightLeftColor;

// Sağ Oda Lambası (Noktasal Işık)
uniform vec3 roomLightRightPos;
uniform vec3 roomLightRightColor;

uniform bool isFlame;

// Yeni Gelişmiş Efekt Uniformları
uniform vec3 skyColor;
uniform float time;
uniform bool isWater;
uniform bool isWall;

// Bir noktanın belirli bir kutunun içinde olup olmadığını kontrol eden fonksiyon (Pencere boşlukları için)
bool insideBox(vec3 p, vec3 center, vec3 size) {
    vec3 halfSize = size * 0.5;
    return p.x >= center.x - halfSize.x && p.x <= center.x + halfSize.x &&
           p.y >= center.y - halfSize.y && p.y <= center.y + halfSize.y &&
           p.z >= center.z - halfSize.z && p.z <= center.z + halfSize.z;
}

void main()
{
    // 1. Eğer duvarsa ve pencere konumlarındaysa bu pikselleri çizme (Duvarlarda gerçek pencere boşluğu açar)
    // Kalınlık değeri (z ve x için) 0.3'e çıkarılarak iç panel ve dış duvarın her ikisinin birden delinmesi sağlandı.
    if (isWall) {
        if (insideBox(FragPos, vec3(-1.5, 0.3, 2.0), vec3(0.52, 0.52, 0.3)) ||
            insideBox(FragPos, vec3(1.7, 0.3, 2.0), vec3(0.52, 0.52, 0.3)) ||
            insideBox(FragPos, vec3(-2.5, 0.3, 0.0), vec3(0.3, 0.52, 0.52)) ||
            insideBox(FragPos, vec3(2.5, 0.3, 0.8), vec3(0.3, 0.52, 0.52)) ||
            insideBox(FragPos, vec3(-1.5, 0.3, -2.0), vec3(0.52, 0.52, 0.3)) ||
            insideBox(FragPos, vec3(1.0, 0.3, -2.0), vec3(0.52, 0.52, 0.3))) {
            discard;
        }
    }

    vec3 baseColor = objectColor;
    float finalAlpha = objectAlpha;
    
    if(useTexture) {
        vec4 texColor = texture(diffuseMap, TexCoords * uvScale + uvOffset);
        // Dokuyu sRGB'den Lineer Alana geçir (Daha doğru ışık hesaplamaları için)
        vec3 texLinear = pow(texColor.rgb, vec3(2.2));
        baseColor = texLinear * objectColor;
        finalAlpha = texColor.a * objectAlpha;
    }
    
    if(finalAlpha < 0.05) {
        discard;
    }

    vec3 norm = normalize(Normal);
    
    // Kuyu Suyu için Organik Normal Bozunumu (Dalga ripples)
    if(isWater) {
        vec3 waterNorm = norm;
        waterNorm.x += sin(FragPos.x * 12.0 + time * 3.5) * 0.15;
        waterNorm.z += cos(FragPos.z * 12.0 + time * 2.8) * 0.15;
        waterNorm.x += sin(FragPos.x * 24.0 - time * 5.0) * 0.05;
        waterNorm.z += cos(FragPos.z * 24.0 - time * 4.0) * 0.05;
        norm = normalize(waterNorm);
    }
    
    vec3 viewDir = normalize(viewPos - FragPos);

    // ==========================================
    // 1. GÜNEŞ / AY IŞIĞI (Blinn-Phong)
    // ==========================================
    float ambientStrength = 0.15; 
    vec3 ambient = ambientStrength * lightColor;
  	
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;  
        
    vec3 totalLight = ambient + diffuse + specular;

    // ==========================================
    // 2. KAPILI ÜSTÜ LAMBASI (Noktasal Işık - Blinn-Phong)
    // ==========================================
    if(lampOn) {
        vec3 lampDir = normalize(lampPos - FragPos);
        float lampDist = length(lampPos - FragPos);
        float att = 1.0 / (1.0 + 0.7 * lampDist + 1.8 * (lampDist * lampDist));
        
        float lampDiff = max(dot(norm, lampDir), 0.0);
        vec3 lampDiffuse = lampDiff * lampColor * att;
        
        vec3 lampHalfway = normalize(lampDir + viewDir);
        float lampSpec = pow(max(dot(norm, lampHalfway), 0.0), shininess);
        vec3 lampSpecular = specularStrength * lampSpec * lampColor * att;
        
        totalLight += lampDiffuse + lampSpecular;
    }

    // ==========================================
    // 3. ŞÖMİNE ATEŞİ (Noktasal Işık - Blinn-Phong)
    // ==========================================
    vec3 fireDir = normalize(firePos - FragPos);
    float fireDist = length(firePos - FragPos);
    float fireAtt = 1.0 / (1.0 + 0.5 * fireDist + 1.0 * (fireDist * fireDist));
    
    float fireDiff = max(dot(norm, fireDir), 0.0);
    vec3 fireDiffuse = fireDiff * fireColor * fireAtt;
    
    vec3 fireHalfway = normalize(fireDir + viewDir);
    float fireSpec = pow(max(dot(norm, fireHalfway), 0.0), shininess);
    vec3 fireSpecular = specularStrength * fireSpec * fireColor * fireAtt;
    
    totalLight += fireDiffuse + fireSpecular;

    // ==========================================
    // 4. SOL ODA LAMBASI (Ceiling Light - Blinn-Phong)
    // ==========================================
    vec3 rLeftDir = normalize(roomLightLeftPos - FragPos);
    float rLeftDist = length(roomLightLeftPos - FragPos);
    float rLeftAtt = 1.0 / (1.0 + 0.8 * rLeftDist + 1.5 * (rLeftDist * rLeftDist));
    
    float rLeftDiff = max(dot(norm, rLeftDir), 0.0);
    vec3 rLeftDiffuse = rLeftDiff * roomLightLeftColor * rLeftAtt;
    
    vec3 rLeftHalfway = normalize(rLeftDir + viewDir);
    float rLeftSpec = pow(max(dot(norm, rLeftHalfway), 0.0), shininess);
    vec3 rLeftSpecular = specularStrength * rLeftSpec * roomLightLeftColor * rLeftAtt;
    
    totalLight += rLeftDiffuse + rLeftSpecular;

    // ==========================================
    // 5. SAĞ ODA LAMBASI (Ceiling Light - Blinn-Phong)
    // ==========================================
    vec3 rRightDir = normalize(roomLightRightPos - FragPos);
    float rRightDist = length(roomLightRightPos - FragPos);
    float rRightAtt = 1.0 / (1.0 + 0.8 * rRightDist + 1.5 * (rRightDist * rRightDist));
    
    float rRightDiff = max(dot(norm, rRightDir), 0.0);
    vec3 rRightDiffuse = rRightDiff * roomLightRightColor * rRightAtt;
    
    vec3 rRightHalfway = normalize(rRightDir + viewDir);
    float rRightSpec = pow(max(dot(norm, rRightHalfway), 0.0), shininess);
    vec3 rRightSpecular = specularStrength * rRightSpec * roomLightRightColor * rRightAtt;
    
    // ==========================================
    // 6. ÇİT LAMBALARI (Fence Post Lights - 64 adet Noktasal Işık)
    // ==========================================
    if (lampOn) {
        vec3 fenceLightColor = vec3(1.0, 0.78, 0.45); // Sıcak sarı sokak lambası rengi
        float totalFenceDiff = 0.0;
        
        // A) Ön ve Arka Çit Direklerindeki Işıklar (X ekseni boyunca)
        for (int i = 0; i <= 15; ++i) {
            float x = -6.0 + float(i) * 0.8;
            
            // Arka Çit Işıkları
            vec3 lPosRear = vec3(x, 0.16, -6.0);
            float dRear = distance(lPosRear, FragPos);
            if (dRear < 3.2) {
                float att = 1.0 / (1.0 + 3.0 * dRear + 6.0 * (dRear * dRear));
                vec3 lDir = normalize(lPosRear - FragPos);
                totalFenceDiff += max(dot(norm, lDir), 0.0) * att;
            }
            
            // Ön Çit Işıkları (Kapı boşluğu hariç: x = 0.8 ve 1.6 civarı)
            if (x < 0.7 || x > 1.3) {
                vec3 lPosFront = vec3(x, 0.16, 6.0);
                float dFront = distance(lPosFront, FragPos);
                if (dFront < 3.2) {
                    float att = 1.0 / (1.0 + 3.0 * dFront + 6.0 * (dFront * dFront));
                    vec3 lDir = normalize(lPosFront - FragPos);
                    totalFenceDiff += max(dot(norm, lDir), 0.0) * att;
                }
            }
        }
        
        // B) Sol ve Sağ Çit Direklerindeki Işıklar (Z ekseni boyunca)
        for (int i = 0; i <= 15; ++i) {
            float z = -6.0 + float(i) * 0.8;
            
            // Sol Çit Işıkları
            vec3 lPosLeft = vec3(-6.0, 0.16, z);
            float dLeft = distance(lPosLeft, FragPos);
            if (dLeft < 3.2) {
                float att = 1.0 / (1.0 + 3.0 * dLeft + 6.0 * (dLeft * dLeft));
                vec3 lDir = normalize(lPosLeft - FragPos);
                totalFenceDiff += max(dot(norm, lDir), 0.0) * att;
            }
            
            // Sağ Çit Işıkları
            vec3 lPosRight = vec3(6.0, 0.16, z);
            float dRight = distance(lPosRight, FragPos);
            if (dRight < 3.2) {
                float att = 1.0 / (1.0 + 3.0 * dRight + 6.0 * (dRight * dRight));
                vec3 lDir = normalize(lPosRight - FragPos);
                totalFenceDiff += max(dot(norm, lDir), 0.0) * att;
            }
        }
        
        totalLight += totalFenceDiff * fenceLightColor;
    }

    // ==========================================
    // SONUÇ VE TONLAMALAR
    // ==========================================
    vec3 result = totalLight * baseColor;
    if(isFlame) {
        result = baseColor;
    }
    
    // 1. Sis Efekti (Exponential Fog)
    float dist = length(viewPos - FragPos);
    float fogDensity = 0.035; 
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    vec3 colWithFog = mix(skyColor, result, fogFactor);

    // 2. Ekran Alanı Karartma (Cinematic Vignette)
    vec2 ndc = (gl_FragCoord.xy / vec2(800.0, 600.0)) * 2.0 - 1.0;
    float vign = 1.0 - dot(ndc, ndc) * 0.22;
    vec3 colWithVign = colWithFog * clamp(vign, 0.0, 1.0);

    // 3. Gamma Düzeltmesi
    vec3 gammaCorrected = pow(colWithVign, vec3(1.0 / 2.2));
    
    FragColor = vec4(gammaCorrected, finalAlpha);
}
